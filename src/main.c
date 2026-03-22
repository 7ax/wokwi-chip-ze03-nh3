#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include "wokwi-api.h"
#pragma clang diagnostic pop

typedef enum { MODE_ACTIVE = 0, MODE_QA = 1 } sensor_mode_t;

typedef struct {
  uart_dev_t    uart;
  uint32_t      attr_nh3;
  sensor_mode_t mode;
  uint8_t       rx_buf[9];
  uint8_t       rx_count;
  timer_t       tx_timer;
  timer_t       rx_timeout;
  uint8_t       tx_buf[9];     /* persistent TX buffer — survives async uart_write */
  bool          tx_busy;        /* guards against frame interleaving */
  uint32_t      attr_warmup;    /* warmup_ms attribute handle */
  double        init_nanos;     /* simulation time at chip_init */
  uint32_t      attr_fault;     /* fault attribute handle */
} chip_state_t;

static chip_state_t state;

static uint8_t checksum(const uint8_t *frame) {
  uint8_t sum = 0;
  for (int i = 1; i <= 7; i++) {
    sum += frame[i];
  }
  return (~sum + 1) & 0xFF;
}

static uint16_t read_concentration(chip_state_t *s) {
  uint32_t warmup_ms = attr_read(s->attr_warmup);
  if (warmup_ms > 0) {
    double elapsed = get_sim_nanos_d() - s->init_nanos;
    if (elapsed < (double)warmup_ms * 1e6) {
      return 0;
    }
  }
  float ppm = attr_read_float(s->attr_nh3);
  if (ppm != ppm) ppm = 0.0f;    /* NaN guard — cannot use isnan() under -nostdlib */
  if (ppm < 0.0f) ppm = 0.0f;
  if (ppm > 100.0f) ppm = 100.0f;
  return (uint16_t)ppm;
}

static void send_frame(chip_state_t *s, uint8_t *frame) {
  if (s->tx_busy) return;
  s->tx_busy = true;
  if (!uart_write(s->uart, frame, 9)) {
    s->tx_busy = false;
  }
}

static void build_concentration_frame(uint8_t *frame, uint16_t val) {
  frame[0] = 0xFF;
  frame[1] = 0x86;
  frame[2] = (uint8_t)((val >> 8) & 0xFF);
  frame[3] = (uint8_t)(val & 0xFF);
  frame[4] = 0x00;
  frame[5] = 0x00;
  frame[6] = 0x00;
  frame[7] = 0x00;
  frame[8] = checksum(frame);
}

static void build_mode_response(uint8_t *frame, uint8_t success) {
  frame[0] = 0xFF;
  frame[1] = 0x78;
  frame[2] = success;
  frame[3] = 0x00;
  frame[4] = 0x00;
  frame[5] = 0x00;
  frame[6] = 0x00;
  frame[7] = 0x00;
  frame[8] = checksum(frame);
}

static void build_cal_response(uint8_t *frame, uint8_t cmd) {
  frame[0] = 0xFF;
  frame[1] = cmd;
  frame[2] = 0x01;   /* success */
  frame[3] = 0x00;
  frame[4] = 0x00;
  frame[5] = 0x00;
  frame[6] = 0x00;
  frame[7] = 0x00;
  frame[8] = checksum(frame);
}

static void on_write_done(void *user_data) {
  chip_state_t *s = (chip_state_t *)user_data;
  s->tx_busy = false;
}

static void on_rx_timeout(void *user_data) {
  chip_state_t *s = (chip_state_t *)user_data;
  if (s->rx_count > 0 && s->rx_count < 9) {
    s->rx_count = 0;
  }
}

static void process_rx_frame(chip_state_t *s) {
  uint8_t *b = s->rx_buf;

  /* Defensive guard — unreachable because on_uart_rx filters b[0] at entry.
     Retained as defense-in-depth against future changes to the RX path. */
  if (b[0] != 0xFF) {
    s->rx_count = 0;
    return;
  }

  if (b[8] != checksum(b)) {
    s->rx_count = 0;
    return;
  }

  if (attr_read(s->attr_fault)) {
    s->rx_count = 0;
    return;
  }

  if (b[1] == 0x86) {
    /* Short Q&A read: FF 86 00 00 00 00 00 00 7A */
    uint16_t val = read_concentration(s);
    build_concentration_frame(s->tx_buf, val);
    send_frame(s, s->tx_buf);
  } else if (b[1] == 0x01 && b[2] == 0x86) {
    /* Datasheet Q&A read concentration request */
    uint16_t val = read_concentration(s);
    build_concentration_frame(s->tx_buf, val);
    send_frame(s, s->tx_buf);
  } else if (b[1] == 0x01 && b[2] == 0x78 && b[3] == 0x04) {
    /* Switch to Q&A mode */
    s->mode = MODE_QA;
    build_mode_response(s->tx_buf, 0x01);
    send_frame(s, s->tx_buf);
  } else if (b[1] == 0x01 && b[2] == 0x78 && b[3] == 0x03) {
    /* Switch to Active Upload mode */
    s->mode = MODE_ACTIVE;
    build_mode_response(s->tx_buf, 0x01);
    send_frame(s, s->tx_buf);
  } else if (b[1] == 0x01 && b[2] == 0x87) {
    /* Zero calibration (stub — ACK only) */
    build_cal_response(s->tx_buf, 0x87);
    send_frame(s, s->tx_buf);
  } else if (b[1] == 0x01 && b[2] == 0x88) {
    /* Span calibration (stub — ACK only) */
    build_cal_response(s->tx_buf, 0x88);
    send_frame(s, s->tx_buf);
  }

  s->rx_count = 0;
}

static void on_uart_rx(void *user_data, uint8_t byte) {
  chip_state_t *s = (chip_state_t *)user_data;

  if (s->rx_count == 0 && byte != 0xFF) {
    return;
  }

  if (s->rx_count >= 9) {
    s->rx_count = 0;
    return;
  }

  s->rx_buf[s->rx_count++] = byte;
  timer_start(s->rx_timeout, 100000, false);

  if (s->rx_count >= 9) {
    timer_stop(s->rx_timeout);
    process_rx_frame(s);
  }
}

static void on_tx_timer(void *user_data) {
  chip_state_t *s = (chip_state_t *)user_data;
  if (attr_read(s->attr_fault)) return;
  if (s->mode == MODE_ACTIVE) {
    uint16_t val = read_concentration(s);
    build_concentration_frame(s->tx_buf, val);
    send_frame(s, s->tx_buf);
  }
}

void chip_init(void) {
  chip_state_t *s = &state;

  const uart_config_t uart_cfg = {
    .tx = pin_init("TXD", OUTPUT),
    .rx = pin_init("RXD", INPUT),
    .baud_rate = 9600,
    .rx_data = on_uart_rx,
    .write_done = on_write_done,
    .user_data = s,
  };
  s->uart = uart_init(&uart_cfg);

  s->attr_nh3 = attr_init_float("nh3_ppm", 5.0f);
  s->attr_warmup = attr_init("warmup_ms", 0);
  s->attr_fault = attr_init("fault", 0);
  s->init_nanos = get_sim_nanos_d();
  s->mode = MODE_ACTIVE;
  s->rx_count = 0;
  s->tx_busy = false;

  const timer_config_t tx_cfg = {
    .callback = on_tx_timer,
    .user_data = s,
  };
  s->tx_timer = timer_init(&tx_cfg);
  timer_start(s->tx_timer, 1000000, true);

  const timer_config_t rx_to_cfg = {
    .callback = on_rx_timeout,
    .user_data = s,
  };
  s->rx_timeout = timer_init(&rx_to_cfg);
}

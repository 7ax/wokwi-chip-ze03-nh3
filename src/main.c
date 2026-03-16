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
} chip_state_t;

static chip_state_t state;

static uint8_t checksum(const uint8_t *frame) {
  uint8_t sum = 0;
  for (int i = 1; i <= 7; i++) {
    sum += frame[i];
  }
  return (~sum + 1) & 0xFF;
}

static void send_frame(chip_state_t *s, uint8_t *frame) {
  uart_write(s->uart, frame, 9);
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

static void on_rx_timeout(void *user_data) {
  chip_state_t *s = (chip_state_t *)user_data;
  if (s->rx_count > 0 && s->rx_count < 9) {
    s->rx_count = 0;
  }
}

static void process_rx_frame(chip_state_t *s) {
  uint8_t *b = s->rx_buf;

  if (b[0] != 0xFF) {
    s->rx_count = 0;
    return;
  }

  if (b[8] != checksum(b)) {
    s->rx_count = 0;
    return;
  }

  uint8_t resp[9];

  if (b[1] == 0x01 && b[2] == 0x86) {
    /* Q&A read concentration request */
    float ppm = attr_read_float(s->attr_nh3);
    uint16_t val = (uint16_t)ppm;
    build_concentration_frame(resp, val);
    send_frame(s, resp);
  } else if (b[1] == 0x01 && b[2] == 0x78 && b[3] == 0x04) {
    /* Switch to Q&A mode */
    s->mode = MODE_QA;
    build_mode_response(resp, 0x01);
    send_frame(s, resp);
  } else if (b[1] == 0x01 && b[2] == 0x78 && b[3] == 0x03) {
    /* Switch to Active Upload mode */
    s->mode = MODE_ACTIVE;
    build_mode_response(resp, 0x01);
    send_frame(s, resp);
  }

  s->rx_count = 0;
}

static void on_uart_rx(void *user_data, uint8_t byte) {
  chip_state_t *s = (chip_state_t *)user_data;

  if (s->rx_count == 0 && byte != 0xFF) {
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
  if (s->mode == MODE_ACTIVE) {
    float ppm = attr_read_float(s->attr_nh3);
    uint16_t val = (uint16_t)ppm;
    uint8_t frame[9];
    build_concentration_frame(frame, val);
    send_frame(s, frame);
  }
}

void chip_init(void) {
  chip_state_t *s = &state;

  const uart_config_t uart_cfg = {
    .tx = pin_init("TXD", INPUT_PULLUP),
    .rx = pin_init("RXD", INPUT),
    .baud_rate = 9600,
    .rx_data = on_uart_rx,
    .user_data = s,
  };
  s->uart = uart_init(&uart_cfg);

  s->attr_nh3 = attr_init_float("nh3_ppm", 5.0f);
  s->mode = MODE_ACTIVE;
  s->rx_count = 0;

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

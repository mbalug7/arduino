// inputs
constexpr int8_t THERMOSTAT_TEMPERATURE_HIGH_PIN = A3;
constexpr int8_t THERMOSTAT_TEMPERATURE_LOW_PIN = A4;
constexpr int8_t PREASURE_HIGH_PIN = A1;
constexpr int8_t PREASURE_LOW_PIN = A2;

// outputs
constexpr int8_t MAIN_ENGIN_PIN = 3;
constexpr int8_t COLD_INPUT_PIN = 4;
constexpr int8_t COLD_VACUUM_PIN = 5;
constexpr int8_t HOT_INPUT_PIN = 6;
constexpr int8_t HOT_VACUUM_PIN = 7;
constexpr int8_t HOT_EXTERNAL_LIQUID_PIN = 8;
constexpr int8_t VACUUM_AIR_PIN = 9;
constexpr int8_t HEATER_EN_PIN = 10;
constexpr int8_t PUMP_EN_PIN = 11;

enum InputStates {
  FAIL = 1,
  HIGH_ON = 3,
  LOW_ON = 5,
  UNDEFINED = 6
};

InputStates getSensorState(int8_t high_pin, int8_t low_pin) {
  InputStates state = UNDEFINED;
  const bool state_high = digitalRead(high_pin);
  const bool state_low = digitalRead(low_pin);

  if (state_high == LOW) {
    state = HIGH_ON;
  }

  if (state_low == LOW) {
    state = LOW_ON;
  }

  if (state_high == LOW && state_low == LOW) {
    state = FAIL;
  }

  return state;
}


InputStates thermostat_state = UNDEFINED;
InputStates preassure_state = UNDEFINED;

constexpr unsigned long cold_wash_period = 360000; // 420000 7 min
constexpr unsigned long hot_wash_period = 1800000; // 30 min

bool all_done = false;

void setupInitialPinStates() {
  digitalWrite(MAIN_ENGIN_PIN, HIGH);
  digitalWrite(COLD_INPUT_PIN, LOW);
  digitalWrite(COLD_VACUUM_PIN, LOW);
  digitalWrite(HOT_INPUT_PIN, LOW);
  digitalWrite(HOT_VACUUM_PIN, LOW);
  digitalWrite(HOT_EXTERNAL_LIQUID_PIN, LOW);
  digitalWrite(VACUUM_AIR_PIN, LOW);
  digitalWrite(HEATER_EN_PIN, LOW);
  digitalWrite(PUMP_EN_PIN, LOW);
}

void setup() {
  Serial.begin(9600);
  pinMode(THERMOSTAT_TEMPERATURE_HIGH_PIN, INPUT_PULLUP);
  pinMode(THERMOSTAT_TEMPERATURE_LOW_PIN, INPUT_PULLUP);
  pinMode(PREASURE_HIGH_PIN, INPUT_PULLUP);
  pinMode(PREASURE_LOW_PIN, INPUT_PULLUP);

  // put your setup code here, to run once:
  pinMode(MAIN_ENGIN_PIN, OUTPUT);
  pinMode(COLD_INPUT_PIN, OUTPUT);
  pinMode(COLD_VACUUM_PIN, OUTPUT);
  pinMode(HOT_INPUT_PIN, OUTPUT);
  pinMode(HOT_VACUUM_PIN, OUTPUT);
  pinMode(HOT_EXTERNAL_LIQUID_PIN, OUTPUT);
  pinMode(VACUUM_AIR_PIN, OUTPUT);
  pinMode(HEATER_EN_PIN, OUTPUT);
  pinMode(PUMP_EN_PIN, OUTPUT);
  setupInitialPinStates();
}

void loop() {
  // put your main code here, to run repeatedly:

  thermostat_state = getSensorState(THERMOSTAT_TEMPERATURE_HIGH_PIN, THERMOSTAT_TEMPERATURE_LOW_PIN);
  preassure_state = getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN);

  if (!all_done) {
    bool has_err = checkForErrors(0, 0);
    if (!has_err) {
      Serial.println("Cold");
      has_err = performColdWaterWash();
    }
    performWaitForAction(40000);
    if (!has_err) {
      Serial.println("Extra liquid");
      has_err = performGetExternalLiquid(60000);
    }
    if (!has_err) {
      Serial.println("HOT WATER");
      has_err = performHotWaterWash();
    }
    performWaitForAction(40000);
    if (!has_err) {
      Serial.println("COLD WATER");
      has_err = performColdWaterWash();
    }
    performWaitForAction(40000);
    if (!has_err) {
      has_err = performAirCleaning(60000);
    }
    all_done = true;
  }
}


bool performColdWaterWash() {

  performEngineStart();
  unsigned long end_time = millis() + cold_wash_period;
  digitalWrite(COLD_VACUUM_PIN, HIGH);
  digitalWrite(COLD_INPUT_PIN, HIGH);
  unsigned long start_time = millis();
  bool can_finish = false;
  while (millis() < end_time) {

    if (getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) == LOW_ON) {
      digitalWrite(COLD_INPUT_PIN, HIGH);
    }

    if (millis() - start_time > 180000 && getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) == HIGH_ON) {
      digitalWrite(COLD_INPUT_PIN, LOW);
    }
    if (checkForErrors(0, 0)) {
      return true;
    }
  }
  digitalWrite(COLD_INPUT_PIN, LOW);
  digitalWrite(COLD_VACUUM_PIN, LOW);
  performAirCleaning(30000);
  digitalWrite(MAIN_ENGIN_PIN, HIGH);
  setupInitialPinStates();
  return false;
}

bool performSystemFlushing() {
  digitalWrite(COLD_VACUUM_PIN, HIGH);
  digitalWrite(PUMP_EN_PIN, HIGH);
  delay(500);
  digitalWrite(COLD_VACUUM_PIN, LOW);
  digitalWrite(PUMP_EN_PIN, LOW);
  return false;
}


bool performGetExternalLiquid(unsigned long get_external_fluid_period) {
    performEngineStart();
  unsigned long end_time = millis() + get_external_fluid_period;
  digitalWrite(HOT_VACUUM_PIN, HIGH);
  digitalWrite(HOT_EXTERNAL_LIQUID_PIN, HIGH);
  while (millis() < end_time) {
    if (checkForErrors(0, 0)) {
      return true;
    }
  }
  digitalWrite(HOT_EXTERNAL_LIQUID_PIN, LOW);
  end_time = millis() + get_external_fluid_period / 2;
  while (millis() < end_time) {
    if (checkForErrors(0, 0)) {
      return true;
    }
  }
  return false;
}


bool performHotWaterWash() {
    performEngineStart();
  unsigned long end_time = millis() + hot_wash_period;
  digitalWrite(HOT_VACUUM_PIN, HIGH);
  digitalWrite(HOT_EXTERNAL_LIQUID_PIN, HIGH);

  while (millis() < end_time) {

    if (getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) == LOW_ON) {
      digitalWrite(HOT_INPUT_PIN, HIGH);
    }

    if (getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) == HIGH_ON) {
      digitalWrite(HOT_INPUT_PIN, LOW);
    }

    if (getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) != LOW_ON && getSensorState(THERMOSTAT_TEMPERATURE_HIGH_PIN, THERMOSTAT_TEMPERATURE_LOW_PIN) == LOW_ON) {
      const unsigned long water_heating_started = millis();
      digitalWrite(HEATER_EN_PIN, HIGH);
      while (getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) != LOW_ON && getSensorState(THERMOSTAT_TEMPERATURE_HIGH_PIN, THERMOSTAT_TEMPERATURE_LOW_PIN) != HIGH_ON && millis() < end_time + hot_wash_period) {
        if (checkForErrors(water_heating_started, 300000)) {
          return true;
        }
        if (getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) == LOW_ON) {
          digitalWrite(HOT_INPUT_PIN, HIGH);
        }

        if (getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN) == HIGH_ON) {
          digitalWrite(HOT_INPUT_PIN, LOW);
        }
      }
      digitalWrite(HEATER_EN_PIN, LOW);
      end_time = end_time +  (millis() - water_heating_started);
    }
    if (checkForErrors(0, 0)) {
      return true;
    }
  }
  digitalWrite(HOT_EXTERNAL_LIQUID_PIN, LOW);
  digitalWrite(HOT_INPUT_PIN, LOW);
  digitalWrite(HOT_VACUUM_PIN, LOW);
  performAirCleaning(30000);
  setupInitialPinStates();
  return false;
}

bool performAirCleaning(unsigned long air_cleaning_period) {
    performEngineStart();
  unsigned long end_time = millis() + air_cleaning_period;
  digitalWrite(VACUUM_AIR_PIN, HIGH);
  digitalWrite(COLD_VACUUM_PIN, HIGH);
  while (millis() < end_time) {
    if (checkForErrors(0, 0)) {
      return true;
    }
  }
  digitalWrite(VACUUM_AIR_PIN, LOW);
  performSystemFlushing();
  while (millis() < end_time / 2) {
    if (checkForErrors(0, 0)) {
      return true;
    }
  }
  performSystemFlushing();
  setupInitialPinStates();
  return false;
}

void performEngineStart() {
  if (digitalRead(MAIN_ENGIN_PIN) == HIGH) {
    digitalWrite(MAIN_ENGIN_PIN, LOW);
    performWaitForAction(30000);
  }
}

bool performWaitForAction(unsigned long delay_period) {
  unsigned long end_time = millis() + delay_period;
  while (millis() < end_time) {
    if (checkForErrors(0, 0)) {
      return true;
    }
  }
  return false;
}



bool checkForErrors(const unsigned long action_started_time, const unsigned long action_max_performing_period) {
  thermostat_state = getSensorState(THERMOSTAT_TEMPERATURE_HIGH_PIN, THERMOSTAT_TEMPERATURE_LOW_PIN);
  preassure_state = getSensorState(PREASURE_HIGH_PIN, PREASURE_LOW_PIN);
  //  if(thermostat_state == FAIL) {
  //    setupInitialPinStates();
  //    return true;
  //  }
  //  if(preassure_state == FAIL) {
  //    setupInitialPinStates();
  //    return true;
  //  }
  return false;
}

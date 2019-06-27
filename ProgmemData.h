#include <avr/pgmspace.h>

// Full day names
const char d_0[] PROGMEM = "Sunday";
const char d_1[] PROGMEM = "Monday";
const char d_2[] PROGMEM = "Tuesday";
const char d_3[] PROGMEM = "Wed";
const char d_4[] PROGMEM = "Thursday";
const char d_5[] PROGMEM = "Friday";
const char d_6[] PROGMEM = "Saturday";

const char *const daysfull[] PROGMEM = {d_0, d_1, d_2, d_3, d_4, d_5, d_6};

// Full month names
const char m_0[] PROGMEM = "January";
const char m_1[] PROGMEM = "February";
const char m_2[] PROGMEM = "March";
const char m_3[] PROGMEM = "April";
const char m_4[] PROGMEM = "May";
const char m_5[] PROGMEM = "June";
const char m_6[] PROGMEM = "July";
const char m_7[] PROGMEM = "August";
const char m_8[] PROGMEM = "Sept";
const char m_9[] PROGMEM = "October";
const char m_10[] PROGMEM = "November";
const char m_11[] PROGMEM = "December";

const char *const monthsfull[] PROGMEM = {m_0, m_1, m_2, m_3, m_4, m_5, m_6, m_7, m_8, m_9, m_10, m_11};

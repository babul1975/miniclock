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

// Words 1 - 19
const char n_0[] PROGMEM = "one";
const char n_1[] PROGMEM = "two";
const char n_2[] PROGMEM = "three";
const char n_3[] PROGMEM = "four";
const char n_4[] PROGMEM = "five";
const char n_5[] PROGMEM = "six";
const char n_6[] PROGMEM = "seven";
const char n_7[] PROGMEM = "eight";
const char n_8[] PROGMEM = "nine";
const char n_9[] PROGMEM = "ten";
const char n_10[] PROGMEM = "eleven";
const char n_11[] PROGMEM = "twelve";
const char n_12[] PROGMEM = "thirteen";
const char n_13[] PROGMEM = "fourteen";
const char n_14[] PROGMEM = "fifteen";
const char n_15[] PROGMEM = "sixteen";
const char n_16[] PROGMEM = "sevent'n";
const char n_17[] PROGMEM = "eighteen";
const char n_18[] PROGMEM = "nineteen";

const char *const numbers[] PROGMEM = {n_0, n_1, n_2, n_3, n_4, n_5, n_6, n_7, n_8, n_9, n_10, n_11, n_12, n_13, n_14, n_15, n_16, n_17, n_18};

// Words tens
const char t_0[] PROGMEM = "ten";
const char t_1[] PROGMEM = "twenty";
const char t_2[] PROGMEM = "thirty";
const char t_3[] PROGMEM = "forty";
const char t_4[] PROGMEM = "fifty";

const char *const numberstens[] PROGMEM = {t_0, t_1, t_2, t_3, t_4};

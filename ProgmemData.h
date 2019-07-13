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

// Main menu
const char mm_0[] PROGMEM = ">Basic";
const char mm_1[] PROGMEM = ">Small";
const char mm_2[] PROGMEM = ">Slide";
const char mm_3[] PROGMEM = ">Words";
const char mm_4[] PROGMEM = ">Setup";

const char *const mainmenu[] PROGMEM = {mm_0, mm_1, mm_2, mm_3, mm_4};

// Clock Set menu
const char c_0[] PROGMEM = ">Set Min";
const char c_1[] PROGMEM = ">Set Hr";
const char c_2[] PROGMEM = ">Set Day";
const char c_3[] PROGMEM = ">Set Mth";
const char c_4[] PROGMEM = ">Set Yr";

const char *const clockset[] PROGMEM = {c_0, c_1, c_2, c_3, c_4};

// Setup menu
const char s_0[] PROGMEM = ">Rnd Clk";
const char s_1[] PROGMEM = ">Rnd Fnt";
const char s_2[] PROGMEM = ">12 Hr";
const char s_3[] PROGMEM = ">Font";
const char s_4[] PROGMEM = ">NTP DST";
const char s_5[] PROGMEM = ">D/Time";
const char s_6[] PROGMEM = ">Auto LX";
const char s_7[] PROGMEM = ">Bright";
const char s_8[] PROGMEM = ">Exit";

const char *const setupmenu[] PROGMEM = {s_0, s_1, s_2, s_3, s_4, s_5, s_6, s_7, s_8};

// Bool menu
const char b_0[] PROGMEM = ">Set DST";
const char b_1[] PROGMEM = ">Set NTP";
const char b_2[] PROGMEM = ">Set 12h";
const char b_3[] PROGMEM = ">Set Rnd";
const char b_4[] PROGMEM = ">Set LX";

const char *const boolmenu[] PROGMEM = {b_0, b_1, b_2, b_3, b_4};

// Display Options
const char o_0[] PROGMEM = ">Normal";
const char o_1[] PROGMEM = ">On";
const char o_2[] PROGMEM = ">Off";
const char o_3[] PROGMEM = "> 9.00pm";
const char o_4[] PROGMEM = ">10.00pm";
const char o_5[] PROGMEM = ">11.00pm";

const char *const displayoptions[] PROGMEM = {o_0, o_1, o_2, o_3, o_4, o_5};

// Date suffix
const char ds_0[] PROGMEM = "st";
const char ds_1[] PROGMEM = "nd";
const char ds_2[] PROGMEM = "rd";
const char ds_3[] PROGMEM = "th";

const char *const suffix[] PROGMEM = {ds_0, ds_1, ds_2, ds_3};

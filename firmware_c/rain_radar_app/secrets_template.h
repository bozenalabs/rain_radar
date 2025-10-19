#pragma once

// Rename this file to secrets.h and fill in your WiFi credentials
// secrets.h is in .gitignore so it won't easily be committed to git

const int NUM_KNOWN_SSIDS = 3;
const char KNOWN_SSIDS[NUM_KNOWN_SSIDS][32] = {
    "asdfasdf",
    "qweqasdqwe",
    "qweqwe1231qwe",
};

const char KNOWN_WIFI_PASSWORDS[NUM_KNOWN_SSIDS][64] = {
    "asdfaewer",
    "asdfqwer",
    "werqweqwe",
};

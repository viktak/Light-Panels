#ifndef TIMECHANGERULES_H
#define TIMECHANGERULES_H

//  Timezones source:
//  https://www.timeanddate.com/time/zones/

#include <Timezone.h>

namespace timechangerules
{
    //  Hawaii
    static TimeChangeRule HST = {"HST", Fourth, Sun, Mar, 2, -600};
    static Timezone Hawaii(HST);

    //  Alaska
    static TimeChangeRule AKDT = {"AKDT", Second, Sun, Mar, 2, -480};
    static TimeChangeRule AKST = {"AKST", First, Sun, Nov, 2, -540};
    static Timezone Alaska(AKDT, AKST);

    //  Pacifica
    static TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};
    static TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};
    static Timezone Pacifica(PDT, PST);

    //  Central US
    static TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};
    static TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};
    static Timezone CentralUS(CDT, CST);

    //  Mountain US
    static TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};
    static TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};
    static Timezone MountainUS(MDT, MST);

    //  Eastern US
    static TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};
    static TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};
    static Timezone EasternUS(EDT, EST);

    //  Cayman Islands
    static TimeChangeRule CIST = {"CIST", First, Sun, Nov, 2, -300};
    static Timezone CaymanIslands(CIST);

    //  Cuba
    static TimeChangeRule CubaCDT = {"CDT", Second, Sun, Mar, 0, -240};
    static TimeChangeRule CubaCST = {"CST", First, Sun, Nov, 0, -300};
    static Timezone Cuba(CubaCDT, CubaCST);

    //  Argentina
    static TimeChangeRule ART = {"ART", First, Sun, Apr, 3, -180};
    static Timezone Argentina(ART);

    //  Brasilia
    static TimeChangeRule BRT = {"BRT", First, Sun, Apr, 3, -180};
    static Timezone Brasilia(BRT);

    //  Azores
    static TimeChangeRule AZOST = {"AZOST", Fourth, Sun, Mar, 0, 0};
    static TimeChangeRule AZOT = {"AZOT", Fourth, Sun, Oct, 1, -60};
    static Timezone Azores(AZOST, AZOT);

    //  Western Europe
    static TimeChangeRule WEST = {"WEST", Fourth, Sun, Mar, 2, 60};
    static TimeChangeRule WET = {"WET", Fourth, Sun, Oct, 2, 0};
    static Timezone WesternEurope(WEST, WET);

    //  Central Europe
    static TimeChangeRule CEST = {"CEST", Fourth, Sun, Mar, 2, 120};
    static TimeChangeRule CET = {"CET", Fourth, Sun, Oct, 2, 60};
    static Timezone CentralEurope(CEST, CET);

    //  Eastern Europe
    static TimeChangeRule EEST = {"EEST", Fourth, Sun, Mar, 2, 180};
    static TimeChangeRule EET = {"EET", Fourth, Sun, Oct, 2, 120};
    static Timezone EasternEurope(EEST, EET);

    //  Further-Eastern Europe
    static TimeChangeRule FET = {"FET", Fourth, Sun, Mar, 2, 180};
    static Timezone FurtherEasternEurope(FET);

    //  Moscow
    static TimeChangeRule MSK = {"MSK", Fourth, Sun, Mar, 2, 240};
    static TimeChangeRule MSD = {"MSD", Fourth, Sun, Oct, 2, 180};
    static Timezone Moscow(MSK, MSD);

    //  Turkey
    static TimeChangeRule TRT = {"TRT", Fourth, Sun, Mar, 2, 180};
    static Timezone Turkey(TRT);

    //  Maldives
    static TimeChangeRule MVT = {"MVT", First, Sun, Apr, 3, 300};
    static Timezone Maldives(MVT);

    //  Pakistan
    static TimeChangeRule PKT = {"PKT", First, Sun, Apr, 3, 300};
    static Timezone Pakistan(PKT);

    //  India
    static TimeChangeRule IST = {"IST", First, Sun, Apr, 3, 330};
    static Timezone India(IST);

    //  Western Indonesia
    static TimeChangeRule WIB = {"WIB", First, Sun, Apr, 3, 420};
    static Timezone WesternIndonesia(WIB);

    //  Central Indonesia
    static TimeChangeRule WITA = {"WITA", First, Sun, Apr, 3, 480};
    static Timezone CentralIndonesia(WITA);

    //  Eastern Indonesia
    static TimeChangeRule WIT = {"WIT", First, Sun, Apr, 3, 540};
    static Timezone EasternIndonesia(WIT);

    //  Hong Kong
    static TimeChangeRule HKT = {"HKT", First, Sun, Apr, 3, 480};
    static Timezone HongKong(HKT);

    //  Malaysia
    static TimeChangeRule MYT = {"MYT", First, Sun, Apr, 3, 480};
    static Timezone Malaysia(MYT);

    //  Philippines
    static TimeChangeRule PHT = {"PHT", First, Sun, Apr, 3, 480};
    static Timezone Philippines(PHT);

    //  Singapore
    static TimeChangeRule SGT = {"SGT", First, Sun, Apr, 3, 480};
    static Timezone Singapore(SGT);

    //  Japan
    static TimeChangeRule JST = {"JST", First, Sun, Apr, 3, 540};
    static Timezone Japan(JST);

    //  Korea
    static TimeChangeRule KST = {"KST", First, Sun, Apr, 3, 540};
    static Timezone Korea(KST);

    //  Eastern Australia
    static TimeChangeRule AEST = {"AEST", First, Sun, Apr, 3, 600};
    static TimeChangeRule AEDT = {"AEDT", First, Sun, Oct, 2, 660};
    static Timezone EasternAustralia(AEST, AEDT);

    //  New Zealand
    static TimeChangeRule NZDT = {"NZDT", First, Sun, Apr, 3, 780};
    static TimeChangeRule NZST = {"NZST", First, Sun, Oct, 2, 720};
    static Timezone NewZealand(NZDT, NZST);

    static Timezone *timezones[] = {
        &Hawaii, &Alaska, &Pacifica, &CentralUS, &MountainUS, &EasternUS, &CaymanIslands, &Cuba, &Argentina, &Brasilia, &Azores,
        &WesternEurope, &CentralEurope, &EasternEurope, &FurtherEasternEurope, &Moscow,
        &Turkey, &Maldives, &Pakistan, &India, &WesternIndonesia, &CentralIndonesia, &EasternIndonesia,
        &HongKong, &Malaysia, &Philippines, &Singapore, &Japan, &Korea, &EasternAustralia, &NewZealand};

    static String tzDescriptions[] = {
        "Hawaii", "Alaska", "Pacifica", "Central US", "Mountain US", "Eastern US", "Cayman Islands", "Cuba", "Argentina", "Brasilia", "Azores",
        "Western Europe", "Central Europe", "Eastern Europe", "Further Eastern Europe", "Moscow",
        "Turkey", "Maldives", "Pakistan", "India", "Western Indonesia", "Central Indonesia", "Eastern Indonesia",
        "HongKong", "Malaysia", "Philippines", "Singapore", "Japan", "Korea", "Eastern Australia", "New Zealand"};
}

#endif
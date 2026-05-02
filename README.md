# DMDESP_P10_MorphingClock
used AI to create a Morphing Clock with DMDESP library from https://github.com/busel7/DMDESP/

Article: 
- https://nicuflorica.blogspot.com/2026/04/ceas-ntp-cu-animatii-pe-afisaj-led.html
- https://nicuflorica.blogspot.com/2026/04/ceas-ntp-cu-animatii-pe-afisaj-led_29.html

Note about software versions:
- version 0b.ino => thick 7-segment numbers for a Morphing Clock (HH:MM and "beat" the seconds) -> not too much animation
- version 3c.ino => Morphing Clock HH:MM:SS
- version 3d.ino => Morphing Clock HH:MM:SS and data as static text (DD.MM)
- version 3g => Morphing Clock Hh:MM:SS and scolling custom font (5 pixels height) with name of the day and calendar data (DD.MM.20YY), once in english, once in romanian
- version 3g1 => Morphing Clock Hh:MM:SS and scolling other custom font (5 pixels height) with name of the day and calendar data (DD.MM.20YY), once in english, once in romanian
- version 4b1 - v.4 - open-meteo.com info also using AI (but just for basic sketch)

![real1](https://blogger.googleusercontent.com/img/a/AVvXsEgTMtHDiLlFj5Fy18lJsEThkqLcjMvP5T-KIxAiy9-TK7_F8OO8O05jzm7rVy6Jp3wcnjZ7XUaTa_RpsxquNmQnpF05F6ovP2n4Hf8DycMhU10n5PNmdWPP3scdyS9ztLXT7Z-ZtQPQwzNQnVQpmpiBMxYoFtiwZ7wLS0AkuTnP6opc3upiDnOj-v2w_F0w=w200-h93)
![real2](https://blogger.googleusercontent.com/img/a/AVvXsEhDqv0YiuXXsn5T3ug74bjbzzd2BCNWwUaY9rZ4_ImRB6EkvVRgBLAxDv9q2yRf5OFcT-_ZkWqXb4FvzF4Mo8-4k9kXzjl7Bsk9EblaBGQQsPj8S8XWuKw4mokBJpAgW0zJ8H4SBZ72TSBBEsUpZeYnD1DDzPkwUjMel8YgcWSf4MTs06IJd730T1qVXcNr=w200-h93)
![real3](https://blogger.googleusercontent.com/img/a/AVvXsEja5IW84KqV6VqhPSmJim32qjWsaFFnum_9RGzEf5J0MkYr4UGg9vhxCHHtLkKM42cPJNt_cth7NYB7G2hnAsyEzPk99r4ENmTVfXn9KQqCj4PHv5yrfx_k_6kBWJ5ctYKCY4YfDwwKv_DWsA3ZK9-N_Y2G8Nl70L8hwKaabozyasapUqLDoUFh_czvIS3l=w200-h93)
![gif](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEhQ4T3-eZwylnl89cia3g2nm7cTKatRpxFX27R5tqVYUaCvqX7l5sAEvb6NORTr-Qwfg-cqqdTnAO15LWCBIv0vO-HjBvAShCo_HibshPrSWAvMd6-yQ7IUccAgJNCWWLCAdTc_AeEQZo7GtWcZ20AAYWc2nSu1Kl0LHNZ09mx6FqVhZL6eqnoKZNdOXbqt/w166-h94/Morphing_Clock_on_monochrome_P10_HUB12_display_more_complex_.gif)

![schematic](https://github.com/tehniq3/DMDESP-P10-display/raw/main/ESP8266_P10_DMDESP_wirring.png)




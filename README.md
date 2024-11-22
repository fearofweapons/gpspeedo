# gpspeedo
A GPS Speedo written for the lilygo esp32 S3 T-Display dev board and serial GPS module

https://lilygo.cc/products/t-display-s3?variant=42284559827125

Requires the following libraries ( which can be installed via library manager )

1) https://github.com/mikalhart/TinyGPSPlus
2) https://github.com/LennartHennigs/Button2
3) https://github.com/Bodmer/TFT_eSPI

If you install via library manager or do an Arduino libraries update you WILL have to edit the 

    "User_Setup_Select.h" 
    
to comment-out the line:

    "//#include <User_Setup.h> // Default setup is root library folder"

AND un-comment:

    "#include <User_Setups/Setup206_LilyGo_T_Display_S3.h> // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT"

which is about halfway down the huge file of setups.

See https://github.com/teastainGit/LilyGO-T-display-S3-setup-and-examples/blob/main/T-DisplayS3_Setup.txt for information.

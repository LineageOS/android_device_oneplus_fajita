# Device Tree for OnePlus 6T (fajita)

The OnePlus 6T (codenamed _"fajita"_) is a flagship smartphone from OnePlus.
It was released in November 2018.

| Basic                   | Spec Sheet                                                                                                                     |
| -----------------------:|:------------------------------------------------------------------------------------------------------------------------------ |
| CPU                     | Octa-core (4x2.8 GHz Kryo 385 Gold & 4x1.7 GHz Kryo 385 Silver)                                                                |
| Chipset                 | Qualcomm SDM845 Snapdragon 845                                                                                                 |
| GPU                     | Adreno 630                                                                                                                     |
| Memory                  | 6/8 GB RAM                                                                                                                     |
| Shipped Android Version | 9                                                                                                                              |
| Storage                 | 64/128/256 GB                                                                                                                  |
| Battery                 | Non-removable Li-Po 3700 mAh battery                                                                                           |
| Display                 | Optic AMOLED, 1080 x 2340 pixels, 19:9 ratio (~402 ppi density)                                                                |
| Camera (Back)           | Dual: 16 MP (f/1.7, 27mm, 1/2.6", 1.22µm, gyro-EIS, OIS) + 20 MP (16 MP effective, f/1.7, 1/2.8", 1.0µm), PDAF, dual-LED flash |
| Camera (Front)          | 16 MP (f/2.0, 25mm, 1/3", 1.0µm), gyro-EIS, Auto HDR, 1080p                                                                    |

Copyright 2018 - The LineageOS Project.

![OnePlus 6T](https://cdn2.gsmarena.com/vv/pics/oneplus/oneplus-6t-thunder-purple-1.jpg "OnePlus 6T")

## Temporary build instructions

```
# Compiling
$ m[ake|ka] bootimage systemimage

# Installing
$ fastboot --disable-verity --disable-verification flash vbmeta stock_vbmeta.img
$ fastboot flash boot boot.img
$ fastboot flash system system.img
$ fastboot -w reboot
```

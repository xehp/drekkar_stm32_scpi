EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "RS-232 Opto fibre transiever"
Date "2021-04-21"
Rev ""
Comp "www.eit.se"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 "Copuright (C) 2020 Henrik Björkman"
$EndDescr
$Comp
L opto_board-rescue:LED-Device D6
U 1 1 5E11B181
P 4300 1800
F 0 "D6" V 4338 1683 50  0000 R CNN
F 1 "LED" V 4247 1683 50  0000 R CNN
F 2 "LED_THT:LED_D3.0mm" H 4300 1800 50  0001 C CNN
F 3 "~" H 4300 1800 50  0001 C CNN
	1    4300 1800
	0    -1   -1   0   
$EndComp
$Comp
L opto_board-rescue:D_Zener-Device D8
U 1 1 5E171B70
P 5200 7250
F 0 "D8" V 5154 7329 50  0000 L CNN
F 1 "D_Zener" V 5245 7329 50  0000 L CNN
F 2 "Diode_THT:D_DO-41_SOD81_P3.81mm_Vertical_AnodeUp" H 5200 7250 50  0001 C CNN
F 3 "~" H 5200 7250 50  0001 C CNN
	1    5200 7250
	0    1    1    0   
$EndComp
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR06
U 1 1 5E171B76
P 5200 7450
F 0 "#PWR06" H 5200 7200 50  0001 C CNN
F 1 "GND" H 5205 7277 50  0000 C CNN
F 2 "" H 5200 7450 50  0001 C CNN
F 3 "" H 5200 7450 50  0001 C CNN
	1    5200 7450
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:nucleo_nano-eit_proprietary U2
U 1 1 5E112947
P 4850 4500
F 0 "U2" H 5250 4550 50  0000 C CNN
F 1 "nucleo_nano" H 4800 2700 50  0000 C CNN
F 2 "eit_footprints:nucleo_nano" H 4850 4500 50  0001 C CNN
F 3 "" H 4850 4500 50  0001 C CNN
	1    4850 4500
	1    0    0    -1  
$EndComp
$Comp
L eit_proprietary:MAX232N-dk_Interface-Drivers-Receivers-Transceivers U1
U 1 1 5E112F41
P 2700 1800
F 0 "U1" H 2700 2404 60  0000 C CNN
F 1 "MAX232N-dk_Interface-Drivers-Receivers-Transceivers" H 2700 2346 0   0000 C CNN
F 2 "digikey-footprints:DIP-16_W7.62mm" H 2900 2000 60  0001 L CNN
F 3 "http://www.ti.com/general/docs/suppproductinfo.tsp?distId=10&gotoUrl=http%3A%2F%2Fwww.ti.com%2Flit%2Fgpn%2Fmax232" H 2900 2100 60  0001 L CNN
F 4 "296-1402-5-ND" H 2900 2200 60  0001 L CNN "Digi-Key_PN"
F 5 "MAX232N" H 2900 2300 60  0001 L CNN "MPN"
F 6 "Integrated Circuits (ICs)" H 2900 2400 60  0001 L CNN "Category"
F 7 "Interface - Drivers, Receivers, Transceivers" H 2900 2500 60  0001 L CNN "Family"
F 8 "http://www.ti.com/general/docs/suppproductinfo.tsp?distId=10&gotoUrl=http%3A%2F%2Fwww.ti.com%2Flit%2Fgpn%2Fmax232" H 2900 2600 60  0001 L CNN "DK_Datasheet_Link"
F 9 "/product-detail/en/texas-instruments/MAX232N/296-1402-5-ND/277048" H 2900 2700 60  0001 L CNN "DK_Detail_Page"
F 10 "IC DUAL EIA-232 DRVR/RCVR 16-DIP" H 2900 2800 60  0001 L CNN "Description"
F 11 "Texas Instruments" H 2900 2900 60  0001 L CNN "Manufacturer"
F 12 "Active" H 2900 3000 60  0001 L CNN "Status"
	1    2700 1800
	1    0    0    -1  
$EndComp
$Comp
L eit_proprietary:DB9_Male_MountingHoles-Connector J2
U 1 1 5E1130C1
P 800 2100
F 0 "J2" H 980 2011 50  0000 L CNN
F 1 "DB9_Male_MountingHoles-Connector" H 750 1550 10  0000 L CNN
F 2 "Connector_Dsub:DSUB-9_Male_Horizontal_P2.77x2.84mm_EdgePinOffset7.70mm_Housed_MountingHolesOffset9.12mm" H 800 2100 50  0001 C CNN
F 3 "" H 800 2100 50  0001 C CNN
	1    800  2100
	-1   0    0    1   
$EndComp
$Comp
L opto_board-rescue:+5V-power-board-rescue #PWR02
U 1 1 5E11AF97
P 5200 7050
F 0 "#PWR02" H 5200 6900 50  0001 C CNN
F 1 "+5V-power" H 5215 7223 50  0000 C CNN
F 2 "" H 5200 7050 50  0001 C CNN
F 3 "" H 5200 7050 50  0001 C CNN
	1    5200 7050
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:C-Device C4
U 1 1 5E121A04
P 3650 2600
F 0 "C4" V 3500 2600 50  0000 C CNN
F 1 "0.1 uF" V 3800 2600 50  0000 C CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 3688 2450 50  0001 C CNN
F 3 "~" H 3650 2600 50  0001 C CNN
	1    3650 2600
	-1   0    0    1   
$EndComp
Wire Wire Line
	1100 1900 1300 1900
Wire Wire Line
	1300 2000 2200 2000
Wire Wire Line
	3200 1900 3300 1900
Wire Wire Line
	3300 1900 3300 2800
Wire Wire Line
	3300 2800 1300 2800
Wire Wire Line
	1300 2800 1300 2100
Wire Wire Line
	1300 2100 1100 2100
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR01
U 1 1 5E1525D4
P 1200 2900
F 0 "#PWR01" H 1200 2650 50  0001 C CNN
F 1 "GND" H 1205 2727 50  0000 C CNN
F 2 "" H 1200 2900 50  0001 C CNN
F 3 "" H 1200 2900 50  0001 C CNN
	1    1200 2900
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 2500 1100 2500
Wire Wire Line
	1200 2500 1200 2900
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR04
U 1 1 5E1A7F28
P 2600 2550
F 0 "#PWR04" H 2600 2300 50  0001 C CNN
F 1 "GND" H 2605 2377 50  0000 C CNN
F 2 "" H 2600 2550 50  0001 C CNN
F 3 "" H 2600 2550 50  0001 C CNN
	1    2600 2550
	1    0    0    -1  
$EndComp
Wire Wire Line
	2600 2550 2600 2500
Wire Wire Line
	5550 4800 6700 4800
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR09
U 1 1 5E1B22AD
P 6700 6650
F 0 "#PWR09" H 6700 6400 50  0001 C CNN
F 1 "GND" H 6705 6477 50  0000 C CNN
F 2 "" H 6700 6650 50  0001 C CNN
F 3 "" H 6700 6650 50  0001 C CNN
	1    6700 6650
	1    0    0    -1  
$EndComp
Wire Wire Line
	3550 6500 3550 5000
Wire Wire Line
	3550 5000 4150 5000
Wire Wire Line
	3200 2000 4300 2000
Wire Wire Line
	4300 2000 4300 1950
Wire Wire Line
	4300 1550 4300 1650
Wire Wire Line
	2600 1200 2600 1300
$Comp
L opto_board-rescue:+5V-power-board-rescue #PWR010
U 1 1 5E1D8E20
P 7300 4400
F 0 "#PWR010" H 7300 4250 50  0001 C CNN
F 1 "+5V-power" H 7315 4573 50  0000 C CNN
F 2 "" H 7300 4400 50  0001 C CNN
F 3 "" H 7300 4400 50  0001 C CNN
	1    7300 4400
	1    0    0    -1  
$EndComp
Wire Wire Line
	7300 4400 7300 5000
Wire Wire Line
	7300 5000 5550 5000
Wire Wire Line
	2200 1900 1300 1900
Connection ~ 1300 1900
Wire Wire Line
	1300 1900 1300 2000
$Comp
L opto_board-rescue:Conn_01x03-Connector_Generic J5
U 1 1 5E1E18D7
P 7400 1200
F 0 "J5" H 7550 1200 50  0000 C CNN
F 1 "Conn_01x03" H 7320 966 0   0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 7400 1200 50  0001 C CNN
F 3 "~" H 7400 1200 50  0001 C CNN
	1    7400 1200
	1    0    0    1   
$EndComp
Wire Wire Line
	1400 2200 2200 2200
Wire Wire Line
	6800 1100 6800 5100
Wire Wire Line
	6800 5100 5550 5100
$Comp
L opto_board-rescue:Conn_01x03-Connector_Generic J4
U 1 1 5E1EB3F0
P 7400 800
F 0 "J4" H 7550 800 50  0000 C CNN
F 1 "Conn_01x03" H 7320 608 0   0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 7400 800 50  0001 C CNN
F 3 "~" H 7400 800 50  0001 C CNN
	1    7400 800 
	1    0    0    1   
$EndComp
Wire Wire Line
	4600 2100 3200 2100
Wire Wire Line
	7000 700  7000 5600
Wire Wire Line
	5550 5200 6900 5200
Wire Wire Line
	6900 5200 6900 1300
Wire Wire Line
	4500 1200 4500 800 
Wire Wire Line
	4500 800  1400 800 
Wire Wire Line
	1400 800  1400 2100
Wire Wire Line
	2200 2100 1400 2100
Connection ~ 1400 2100
Wire Wire Line
	1400 2100 1400 2200
$Comp
L opto_board-rescue:R-Device R4
U 1 1 5E230F14
P 800 1050
F 0 "R4" V 700 1000 50  0000 C CNN
F 1 "100" V 800 1050 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical" V 730 1050 50  0001 C CNN
F 3 "~" H 800 1050 50  0001 C CNN
	1    800  1050
	-1   0    0    1   
$EndComp
Wire Wire Line
	800  1200 800  1500
Wire Wire Line
	1200 2500 1200 800 
Wire Wire Line
	1200 800  800  800 
Wire Wire Line
	800  800  800  900 
Connection ~ 1200 2500
NoConn ~ 1100 1700
NoConn ~ 1100 1800
NoConn ~ 1100 2000
NoConn ~ 1100 2200
NoConn ~ 1100 2300
NoConn ~ 1100 2400
NoConn ~ 5550 4900
NoConn ~ 4150 4900
$Comp
L opto_board-rescue:C-Device C7
U 1 1 5E195706
P 1600 1600
F 0 "C7" H 1485 1554 50  0000 R CNN
F 1 "0.1 uF" H 1485 1645 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 1638 1450 50  0001 C CNN
F 3 "~" H 1600 1600 50  0001 C CNN
	1    1600 1600
	-1   0    0    1   
$EndComp
$Comp
L opto_board-rescue:C-Device C6
U 1 1 5E1958F9
P 1600 1100
F 0 "C6" H 1485 1054 50  0000 R CNN
F 1 "0.1 uF" H 1485 1145 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 1638 950 50  0001 C CNN
F 3 "~" H 1600 1100 50  0001 C CNN
	1    1600 1100
	-1   0    0    1   
$EndComp
$Comp
L opto_board-rescue:C-Device C8
U 1 1 5E195975
P 3550 1100
F 0 "C8" H 3435 1054 50  0000 R CNN
F 1 "0.1 uF" H 3435 1145 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 3588 950 50  0001 C CNN
F 3 "~" H 3550 1100 50  0001 C CNN
	1    3550 1100
	-1   0    0    1   
$EndComp
Wire Wire Line
	3200 1600 3300 1600
Wire Wire Line
	1600 950  1600 900 
Wire Wire Line
	1600 900  2100 900 
Wire Wire Line
	2100 900  2100 1500
Wire Wire Line
	2100 1500 2200 1500
Wire Wire Line
	2200 1600 2000 1600
Wire Wire Line
	2000 1600 2000 1300
Wire Wire Line
	2000 1300 1600 1300
Wire Wire Line
	1600 1300 1600 1250
Wire Wire Line
	2200 1800 1600 1800
Wire Wire Line
	1600 1800 1600 1750
Wire Wire Line
	1600 1450 1600 1400
Wire Wire Line
	1600 1400 1900 1400
Wire Wire Line
	1900 1400 1900 1700
Wire Wire Line
	1900 1700 2200 1700
Wire Wire Line
	5550 5600 7000 5600
Wire Wire Line
	5550 6000 7650 6000
NoConn ~ 5550 6100
NoConn ~ 3200 1800
$Comp
L opto_board-rescue:LED-Device D12
U 1 1 5E2292B9
P 8600 4650
F 0 "D12" V 8638 4533 50  0000 R CNN
F 1 "LED" V 8547 4533 50  0000 R CNN
F 2 "LED_THT:LED_D3.0mm" H 8600 4650 50  0001 C CNN
F 3 "~" H 8600 4650 50  0001 C CNN
	1    8600 4650
	0    -1   -1   0   
$EndComp
Wire Wire Line
	6700 6650 6700 6500
Connection ~ 6700 6500
Wire Wire Line
	6700 6500 3550 6500
$Comp
L opto_board-rescue:LED-Device D2
U 1 1 5E3B06FE
P 4400 7250
F 0 "D2" V 4450 7400 50  0000 R CNN
F 1 "LED" V 4347 7133 50  0000 R CNN
F 2 "LED_THT:LED_D3.0mm" H 4400 7250 50  0001 C CNN
F 3 "~" H 4400 7250 50  0001 C CNN
	1    4400 7250
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4400 7100 4400 7050
Wire Wire Line
	4400 7400 4400 7450
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR013
U 1 1 5E126993
P 7650 4600
F 0 "#PWR013" H 7650 4450 50  0001 C CNN
F 1 "+3.3V-power" H 7665 4773 50  0000 C CNN
F 2 "" H 7650 4600 50  0001 C CNN
F 3 "" H 7650 4600 50  0001 C CNN
	1    7650 4600
	1    0    0    -1  
$EndComp
Wire Wire Line
	7650 4600 7650 6000
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR012
U 1 1 5E13E3D8
P 4300 1550
F 0 "#PWR012" H 4300 1400 50  0001 C CNN
F 1 "+3.3V-power" H 4315 1723 50  0000 C CNN
F 2 "" H 4300 1550 50  0001 C CNN
F 3 "" H 4300 1550 50  0001 C CNN
	1    4300 1550
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR014
U 1 1 5E13EBE4
P 8600 4300
F 0 "#PWR014" H 8600 4150 50  0001 C CNN
F 1 "+3.3V-power" H 8615 4473 50  0000 C CNN
F 2 "" H 8600 4300 50  0001 C CNN
F 3 "" H 8600 4300 50  0001 C CNN
	1    8600 4300
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR07
U 1 1 5E13F307
P 3650 2350
F 0 "#PWR07" H 3650 2200 50  0001 C CNN
F 1 "+3.3V-power" H 3665 2523 50  0000 C CNN
F 2 "" H 3650 2350 50  0001 C CNN
F 3 "" H 3650 2350 50  0001 C CNN
	1    3650 2350
	1    0    0    -1  
$EndComp
Wire Wire Line
	3650 2350 3650 2400
Wire Wire Line
	3650 2850 3650 2800
$Comp
L opto_board-rescue:D_Zener-Device D4
U 1 1 5E1575D8
P 3900 2600
F 0 "D4" V 3854 2679 50  0000 L CNN
F 1 "D_Zener" V 3945 2679 50  0000 L CNN
F 2 "Diode_THT:D_DO-41_SOD81_P3.81mm_Vertical_AnodeUp" H 3900 2600 50  0001 C CNN
F 3 "~" H 3900 2600 50  0001 C CNN
	1    3900 2600
	0    1    1    0   
$EndComp
Wire Wire Line
	3900 2450 3900 2400
Wire Wire Line
	3900 2400 3650 2400
Connection ~ 3650 2400
Wire Wire Line
	3650 2400 3650 2450
Wire Wire Line
	3900 2750 3900 2800
Wire Wire Line
	3900 2800 3650 2800
Connection ~ 3650 2800
Wire Wire Line
	3650 2800 3650 2750
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR0103
U 1 1 5E1B6CCA
P 2600 1200
F 0 "#PWR0103" H 2600 1050 50  0001 C CNN
F 1 "+3.3V-power" H 2615 1373 50  0000 C CNN
F 2 "" H 2600 1200 50  0001 C CNN
F 3 "" H 2600 1200 50  0001 C CNN
	1    2600 1200
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:C-Device C5
U 1 1 5E1C3CE0
P 3800 7250
F 0 "C5" V 3650 7250 50  0000 C CNN
F 1 "0.1 uF" V 3950 7250 50  0000 C CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 3838 7100 50  0001 C CNN
F 3 "~" H 3800 7250 50  0001 C CNN
	1    3800 7250
	-1   0    0    1   
$EndComp
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR0104
U 1 1 5E1C3DCE
P 3800 7050
F 0 "#PWR0104" H 3800 6900 50  0001 C CNN
F 1 "+3.3V-power" H 3815 7223 50  0000 C CNN
F 2 "" H 3800 7050 50  0001 C CNN
F 3 "" H 3800 7050 50  0001 C CNN
	1    3800 7050
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0105
U 1 1 5E1C3E79
P 3800 7450
F 0 "#PWR0105" H 3800 7200 50  0001 C CNN
F 1 "GND" H 3805 7277 50  0000 C CNN
F 2 "" H 3800 7450 50  0001 C CNN
F 3 "" H 3800 7450 50  0001 C CNN
	1    3800 7450
	1    0    0    -1  
$EndComp
Wire Wire Line
	3800 7450 3800 7400
Wire Wire Line
	3800 7050 3800 7100
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0106
U 1 1 5E1DC32D
P 4400 7450
F 0 "#PWR0106" H 4400 7200 50  0001 C CNN
F 1 "GND" H 4405 7277 50  0000 C CNN
F 2 "" H 4400 7450 50  0001 C CNN
F 3 "" H 4400 7450 50  0001 C CNN
	1    4400 7450
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR0107
U 1 1 5E1DC4FB
P 4400 7050
F 0 "#PWR0107" H 4400 6900 50  0001 C CNN
F 1 "+3.3V-power" H 4415 7223 50  0000 C CNN
F 2 "" H 4400 7050 50  0001 C CNN
F 3 "" H 4400 7050 50  0001 C CNN
	1    4400 7050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5200 7050 5200 7100
Wire Wire Line
	5200 7400 5200 7450
Wire Wire Line
	7200 700  7000 700 
Wire Wire Line
	4600 800  7100 800 
Wire Wire Line
	4700 900  7100 900 
Wire Wire Line
	7200 1100 6800 1100
Wire Wire Line
	4500 1200 7100 1200
Wire Wire Line
	6900 1300 7100 1300
Wire Wire Line
	4600 800  4600 2100
$Comp
L opto_board-rescue:Conn_01x03-Connector_Generic J7
U 1 1 5E39CDBB
P 1200 6400
F 0 "J7" H 1120 6075 50  0000 C CNN
F 1 "Conn_01x03" H 1120 6166 50  0000 C CNN
F 2 "Connector_Molex:Molex_KK-254_AE-6410-03A_1x03_P2.54mm_Vertical" H 1200 6400 50  0001 C CNN
F 3 "~" H 1200 6400 50  0001 C CNN
	1    1200 6400
	-1   0    0    1   
$EndComp
$Comp
L opto_board-rescue:Conn_01x03-Connector_Generic J6
U 1 1 5E39D0E5
P 1200 5900
F 0 "J6" H 1120 5575 50  0000 C CNN
F 1 "Conn_01x03" H 1120 5666 50  0000 C CNN
F 2 "Connector_Molex:Molex_KK-254_AE-6410-03A_1x03_P2.54mm_Vertical" H 1200 5900 50  0001 C CNN
F 3 "~" H 1200 5900 50  0001 C CNN
	1    1200 5900
	-1   0    0    1   
$EndComp
Wire Wire Line
	1400 5800 1900 5800
Wire Wire Line
	1900 5800 1900 5500
Wire Wire Line
	1900 6000 1900 6300
Wire Wire Line
	1900 6300 1400 6300
Wire Wire Line
	1900 6000 4150 6000
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0110
U 1 1 5E3B3982
P 1500 6700
F 0 "#PWR0110" H 1500 6450 50  0001 C CNN
F 1 "GND" H 1505 6527 50  0000 C CNN
F 2 "" H 1500 6700 50  0001 C CNN
F 3 "" H 1500 6700 50  0001 C CNN
	1    1500 6700
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 6700 1500 6500
Wire Wire Line
	1500 6500 1400 6500
Wire Wire Line
	1500 6500 1500 6000
Wire Wire Line
	1500 6000 1400 6000
Connection ~ 1500 6500
$Comp
L opto_board-rescue:+5V-power-board-rescue #PWR0111
U 1 1 5E3D5CBE
P 1600 5600
F 0 "#PWR0111" H 1600 5450 50  0001 C CNN
F 1 "+5V-power" H 1615 5773 50  0000 C CNN
F 2 "" H 1600 5600 50  0001 C CNN
F 3 "" H 1600 5600 50  0001 C CNN
	1    1600 5600
	1    0    0    -1  
$EndComp
Wire Wire Line
	1600 5600 1600 5900
Wire Wire Line
	1600 5900 1400 5900
Wire Wire Line
	1600 5900 1600 6400
Wire Wire Line
	1600 6400 1400 6400
Connection ~ 1600 5900
Wire Wire Line
	8600 4800 8600 5800
Wire Wire Line
	5550 5800 8600 5800
$Comp
L opto_board-rescue:Conn_01x03-Connector_Generic J8
U 1 1 5E45E28C
P 10600 5300
F 0 "J8" H 10519 4975 50  0000 C CNN
F 1 "Conn_01x03" H 10519 5066 50  0000 C CNN
F 2 "Connector_Molex:Molex_KK-254_AE-6410-03A_1x03_P2.54mm_Vertical" H 10600 5300 50  0001 C CNN
F 3 "~" H 10600 5300 50  0001 C CNN
	1    10600 5300
	1    0    0    1   
$EndComp
$Comp
L opto_board-rescue:Conn_01x03-Connector_Generic J9
U 1 1 5E469F67
P 10600 5800
F 0 "J9" H 10519 5475 50  0000 C CNN
F 1 "Conn_01x03" H 10519 5566 50  0000 C CNN
F 2 "Connector_Molex:Molex_KK-254_AE-6410-03A_1x03_P2.54mm_Vertical" H 10600 5800 50  0001 C CNN
F 3 "~" H 10600 5800 50  0001 C CNN
	1    10600 5800
	1    0    0    1   
$EndComp
Wire Wire Line
	5550 5400 9000 5400
Wire Wire Line
	9000 5400 9000 5200
Wire Wire Line
	9000 5200 9100 5200
Wire Wire Line
	10400 5700 9600 5700
Wire Wire Line
	9000 5700 9000 5500
Wire Wire Line
	9000 5500 5550 5500
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0112
U 1 1 5E4822FA
P 10300 6000
F 0 "#PWR0112" H 10300 5750 50  0001 C CNN
F 1 "GND" H 10305 5827 50  0000 C CNN
F 2 "" H 10300 6000 50  0001 C CNN
F 3 "" H 10300 6000 50  0001 C CNN
	1    10300 6000
	1    0    0    -1  
$EndComp
Wire Wire Line
	10300 6000 10300 5900
Wire Wire Line
	10300 5900 10400 5900
Wire Wire Line
	10300 5900 10300 5400
Wire Wire Line
	10300 5400 10400 5400
Connection ~ 10300 5900
$Comp
L opto_board-rescue:+3.3V-power-board-rescue #PWR0113
U 1 1 5E49B27F
P 10200 5000
F 0 "#PWR0113" H 10200 4850 50  0001 C CNN
F 1 "+3.3V-power" H 10215 5173 50  0000 C CNN
F 2 "" H 10200 5000 50  0001 C CNN
F 3 "" H 10200 5000 50  0001 C CNN
	1    10200 5000
	1    0    0    -1  
$EndComp
Wire Wire Line
	10200 5000 10200 5300
Wire Wire Line
	10200 5300 10400 5300
Wire Wire Line
	10200 5300 10200 5800
Wire Wire Line
	10200 5800 10400 5800
Connection ~ 10200 5300
$Comp
L opto_board-rescue:R-Device R8
U 1 1 5E4B522F
P 9100 4750
F 0 "R8" V 9000 4700 50  0000 C CNN
F 1 "1k" V 9100 4750 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical" V 9030 4750 50  0001 C CNN
F 3 "~" H 9100 4750 50  0001 C CNN
	1    9100 4750
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:R-Device R9
U 1 1 5E4CEF1C
P 9600 4750
F 0 "R9" V 9500 4700 50  0000 C CNN
F 1 "1k" V 9600 4750 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical" V 9530 4750 50  0001 C CNN
F 3 "~" H 9600 4750 50  0001 C CNN
	1    9600 4750
	1    0    0    -1  
$EndComp
Wire Wire Line
	9100 4600 9100 4450
Wire Wire Line
	9100 4450 9350 4450
Wire Wire Line
	9600 4450 9600 4600
Connection ~ 9350 4450
Wire Wire Line
	9350 4450 9600 4450
Wire Wire Line
	9100 4900 9100 5200
Connection ~ 9100 5200
Wire Wire Line
	9100 5200 10400 5200
Wire Wire Line
	9600 4900 9600 5700
Connection ~ 9600 5700
Wire Wire Line
	9600 5700 9000 5700
Wire Wire Line
	1900 5500 4150 5500
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0115
U 1 1 5E12BEB6
P 3650 2850
F 0 "#PWR0115" H 3650 2600 50  0001 C CNN
F 1 "GND" H 3655 2677 50  0000 C CNN
F 2 "" H 3650 2850 50  0001 C CNN
F 3 "" H 3650 2850 50  0001 C CNN
	1    3650 2850
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:C-Device C9
U 1 1 5E12D248
P 9100 6050
F 0 "C9" H 8985 6004 50  0000 R CNN
F 1 "0.1 uF" H 8985 6095 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 9138 5900 50  0001 C CNN
F 3 "~" H 9100 6050 50  0001 C CNN
	1    9100 6050
	-1   0    0    1   
$EndComp
$Comp
L opto_board-rescue:C-Device C10
U 1 1 5E12D323
P 9600 6000
F 0 "C10" H 9485 5954 50  0000 R CNN
F 1 "0.1 uF" H 9485 6045 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 9638 5850 50  0001 C CNN
F 3 "~" H 9600 6000 50  0001 C CNN
	1    9600 6000
	-1   0    0    1   
$EndComp
Wire Wire Line
	9100 6200 9100 6250
Wire Wire Line
	9100 6250 9350 6250
Wire Wire Line
	9600 6250 9600 6150
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR03
U 1 1 5E136410
P 9350 6300
F 0 "#PWR03" H 9350 6050 50  0001 C CNN
F 1 "GND" H 9355 6127 50  0000 C CNN
F 2 "" H 9350 6300 50  0001 C CNN
F 3 "" H 9350 6300 50  0001 C CNN
	1    9350 6300
	1    0    0    -1  
$EndComp
Wire Wire Line
	9350 6300 9350 6250
Connection ~ 9350 6250
Wire Wire Line
	9350 6250 9600 6250
Wire Wire Line
	9100 5900 9100 5200
Wire Wire Line
	9600 5850 9600 5700
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0108
U 1 1 5E1783A3
P 9750 4300
F 0 "#PWR0108" H 9750 4050 50  0001 C CNN
F 1 "GND" H 9755 4127 50  0000 C CNN
F 2 "" H 9750 4300 50  0001 C CNN
F 3 "" H 9750 4300 50  0001 C CNN
	1    9750 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	9750 4300 9750 4200
Wire Wire Line
	9750 4200 9350 4200
Wire Wire Line
	9350 4200 9350 4450
Wire Wire Line
	8600 4300 8600 4500
$Comp
L opto_board-rescue:D_Bridge_+-AA-Device D7
U 1 1 5E13C1AF
P 1600 3600
F 0 "D7" H 1750 3800 50  0000 L CNN
F 1 "D_Bridge_+-AA" H 1700 3900 50  0000 L CNN
F 2 "Diode_THT:Diode_Bridge_DIP-4_W7.62mm_P5.08mm" H 1600 3600 50  0001 C CNN
F 3 "~" H 1600 3600 50  0001 C CNN
	1    1600 3600
	1    0    0    -1  
$EndComp
Wire Wire Line
	3250 5800 3250 4150
Wire Wire Line
	3250 4150 4700 4150
Wire Wire Line
	4700 4150 4700 900 
Wire Wire Line
	3250 5800 4150 5800
Wire Wire Line
	3550 4800 3550 4350
Wire Wire Line
	3550 4350 4900 4350
Wire Wire Line
	4900 4350 4900 1650
Wire Wire Line
	3550 4800 4150 4800
Wire Wire Line
	3650 4700 3650 4450
Wire Wire Line
	3650 4450 5000 4450
Wire Wire Line
	5000 4450 5000 1550
Wire Wire Line
	3650 4700 4150 4700
$Comp
L opto_board-rescue:Conn_01x02-Connector_Generic J1
U 1 1 5E13D6E9
P 800 3650
F 0 "J1" H 720 3325 50  0000 C CNN
F 1 "Conn_01x02" H 720 3416 50  0000 C CNN
F 2 "Connector_Molex:Molex_KK-254_AE-6410-02A_1x02_P2.54mm_Vertical" H 800 3650 50  0001 C CNN
F 3 "~" H 800 3650 50  0001 C CNN
	1    800  3650
	-1   0    0    1   
$EndComp
Wire Wire Line
	1000 3550 1150 3550
Wire Wire Line
	1150 3550 1150 3250
Wire Wire Line
	1150 3250 1600 3250
Wire Wire Line
	1600 3250 1600 3300
Wire Wire Line
	1000 3650 1150 3650
Wire Wire Line
	1150 3650 1150 3950
Wire Wire Line
	1150 3950 1600 3950
Wire Wire Line
	1600 3950 1600 3900
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0114
U 1 1 5E154435
P 1250 4100
F 0 "#PWR0114" H 1250 3850 50  0001 C CNN
F 1 "GND" H 1255 3927 50  0000 C CNN
F 2 "" H 1250 4100 50  0001 C CNN
F 3 "" H 1250 4100 50  0001 C CNN
	1    1250 4100
	1    0    0    -1  
$EndComp
Wire Wire Line
	1300 3600 1250 3600
Wire Wire Line
	1250 3600 1250 4050
$Comp
L opto_board-rescue:R-Device R1
U 1 1 5E16030F
P 2700 3800
F 0 "R1" V 2600 3750 50  0000 C CNN
F 1 "47" V 2700 3800 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P2.54mm_Vertical" V 2630 3800 50  0001 C CNN
F 3 "~" H 2700 3800 50  0001 C CNN
	1    2700 3800
	1    0    0    -1  
$EndComp
Wire Wire Line
	2700 3600 2700 3650
Wire Wire Line
	2700 3950 2700 4050
Connection ~ 1250 4050
Wire Wire Line
	1250 4050 1250 4100
NoConn ~ 5550 4700
Wire Wire Line
	5550 5700 5900 5700
Wire Wire Line
	5900 5700 5900 3600
$Comp
L opto_board-rescue:C-Device C1
U 1 1 5E13D649
P 3150 3800
F 0 "C1" H 3035 3754 50  0000 R CNN
F 1 "10 nF" H 3035 3845 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 3188 3650 50  0001 C CNN
F 3 "~" H 3150 3800 50  0001 C CNN
	1    3150 3800
	-1   0    0    1   
$EndComp
Wire Wire Line
	3150 3650 3150 3600
Wire Wire Line
	3150 3600 5900 3600
Wire Wire Line
	3150 3950 3150 4050
Wire Wire Line
	1250 4050 2700 4050
Connection ~ 2700 4050
Wire Wire Line
	2700 4050 3150 4050
NoConn ~ 4150 5100
NoConn ~ 4150 5200
NoConn ~ 4150 5300
NoConn ~ 4150 5400
NoConn ~ 4150 5900
NoConn ~ 4150 6100
NoConn ~ 5550 5900
NoConn ~ 5550 5300
$Comp
L opto_board-rescue:C-Device C11
U 1 1 5E1C3C65
P 3900 1500
F 0 "C11" H 3785 1454 50  0000 R CNN
F 1 "0.1 uF" H 3785 1545 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 3938 1350 50  0001 C CNN
F 3 "~" H 3900 1500 50  0001 C CNN
	1    3900 1500
	-1   0    0    1   
$EndComp
Wire Wire Line
	3900 1350 3900 1250
Wire Wire Line
	3900 1250 3700 1250
Wire Wire Line
	3700 1250 3700 1700
Wire Wire Line
	3700 1700 3200 1700
Wire Wire Line
	3300 900  3550 900 
Wire Wire Line
	3550 900  3550 950 
Wire Wire Line
	3300 900  3300 1600
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR05
U 1 1 5E217677
P 3550 1300
F 0 "#PWR05" H 3550 1050 50  0001 C CNN
F 1 "GND" H 3555 1127 50  0000 C CNN
F 2 "" H 3550 1300 50  0001 C CNN
F 3 "" H 3550 1300 50  0001 C CNN
	1    3550 1300
	1    0    0    -1  
$EndComp
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR08
U 1 1 5E26ECD8
P 3900 1700
F 0 "#PWR08" H 3900 1450 50  0001 C CNN
F 1 "GND" H 3905 1527 50  0000 C CNN
F 2 "" H 3900 1700 50  0001 C CNN
F 3 "" H 3900 1700 50  0001 C CNN
	1    3900 1700
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 1700 3900 1650
Wire Wire Line
	3550 1300 3550 1250
Text Notes 10750 5300 0    50   ~ 0
to temp\nsensor 1
Text Notes 10750 5800 0    50   ~ 0
to temp\nsensor 2
Text Notes 700  6450 0    50   ~ 0
pulse \ncounter 1
Text Notes 700  5950 0    50   ~ 0
pulse \ncounter 2
$Comp
L opto_board-rescue:R-Device R5
U 1 1 606FDE75
P 2250 3600
F 0 "R5" V 2150 3550 50  0000 C CNN
F 1 "47" V 2250 3600 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P2.54mm_Vertical" V 2180 3600 50  0001 C CNN
F 3 "~" H 2250 3600 50  0001 C CNN
	1    2250 3600
	0    -1   -1   0   
$EndComp
Connection ~ 3150 3600
Text Notes 7550 1050 0    50   ~ 0
input/output\nserial select
Text Notes 600  2700 0    50   ~ 0
RS232
Wire Wire Line
	1900 3600 2100 3600
Wire Wire Line
	2400 3600 2700 3600
Wire Wire Line
	2700 3600 3150 3600
Connection ~ 2700 3600
Wire Wire Line
	6700 4800 6700 6500
Text Notes 10600 1650 0    50   ~ 0
To opto fibre
Wire Wire Line
	5000 1550 8800 1550
Wire Wire Line
	8500 950  8500 1200
Wire Wire Line
	8500 1200 8500 1700
Connection ~ 8500 1200
$Comp
L opto_board-rescue:+5V-power-board-rescue #PWR0102
U 1 1 5E116347
P 8500 950
F 0 "#PWR0102" H 8500 800 50  0001 C CNN
F 1 "+5V-power" H 8515 1123 50  0000 C CNN
F 2 "" H 8500 950 50  0001 C CNN
F 3 "" H 8500 950 50  0001 C CNN
	1    8500 950 
	1    0    0    -1  
$EndComp
Wire Wire Line
	8200 2250 8200 2100
$Comp
L opto_board-rescue:GND-power-opto_board-rescue #PWR0101
U 1 1 5E111589
P 8200 2250
F 0 "#PWR0101" H 8200 2000 50  0001 C CNN
F 1 "GND" H 8205 2077 50  0000 C CNN
F 2 "" H 8200 2250 50  0001 C CNN
F 3 "" H 8200 2250 50  0001 C CNN
	1    8200 2250
	1    0    0    -1  
$EndComp
Wire Wire Line
	8400 1650 8400 2200
Wire Wire Line
	4900 1650 8400 1650
$Comp
L eit_proprietary:opto_transmitter_broadcom TX1
U 1 1 5E09724C
P 10100 1050
F 0 "TX1" H 10100 1165 50  0000 C CNN
F 1 "opto_transmitter_broadcom" H 10100 1074 50  0000 C CNN
F 2 "eit_footprints:opto_transmitter_broadwell" H 10100 1050 50  0001 C CNN
F 3 "" H 10100 1050 50  0001 C CNN
	1    10100 1050
	1    0    0    -1  
$EndComp
$Comp
L eit_proprietary:opto_receiver_broadcom RX1
U 1 1 5E0970DB
P 10100 1850
F 0 "RX1" H 10100 1965 50  0000 C CNN
F 1 "opto_receiver_broadcom" H 10100 1874 50  0000 C CNN
F 2 "eit_footprints:opto_receiver_broadwell" H 10100 1850 50  0001 C CNN
F 3 "" H 10100 1850 50  0001 C CNN
	1    10100 1850
	1    0    0    -1  
$EndComp
Wire Wire Line
	8500 1200 9700 1200
$Comp
L opto_board-rescue:R-Device R2
U 1 1 5E08D481
P 8900 1900
F 0 "R2" V 8800 1850 50  0000 C CNN
F 1 "1k" V 8900 1900 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical" V 8830 1900 50  0001 C CNN
F 3 "~" H 8900 1900 50  0001 C CNN
	1    8900 1900
	-1   0    0    1   
$EndComp
$Comp
L opto_board-rescue:R-Device R3
U 1 1 5E08D2BB
P 9050 1550
F 0 "R3" V 8950 1400 50  0000 C CNN
F 1 "100" V 9050 1550 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical" V 8980 1550 50  0001 C CNN
F 3 "~" H 9050 1550 50  0001 C CNN
	1    9050 1550
	0    1    1    0   
$EndComp
Wire Wire Line
	9300 1700 9600 1700
Connection ~ 9300 1700
$Comp
L opto_board-rescue:C-Device C2
U 1 1 5E08D0D5
P 9300 1850
F 0 "C2" H 9185 1804 50  0000 R CNN
F 1 "0.1u" H 9185 1895 50  0000 R CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 9338 1700 50  0001 C CNN
F 3 "~" H 9300 1850 50  0001 C CNN
	1    9300 1850
	-1   0    0    1   
$EndComp
Wire Wire Line
	8900 1700 9300 1700
$Comp
L opto_board-rescue:C-Device C3
U 1 1 5E08D050
P 9050 1350
F 0 "C3" V 9000 1250 50  0000 C CNN
F 1 "10n" V 9000 1500 50  0000 C CNN
F 2 "Capacitor_THT:C_Disc_D3.8mm_W2.6mm_P2.50mm" H 9088 1200 50  0001 C CNN
F 3 "~" H 9050 1350 50  0001 C CNN
	1    9050 1350
	0    1    1    0   
$EndComp
Connection ~ 9350 1400
Wire Wire Line
	9350 1350 9350 1400
Wire Wire Line
	9200 1350 9350 1350
Connection ~ 8800 1550
Wire Wire Line
	8800 1350 8800 1550
Wire Wire Line
	8900 1350 8800 1350
Connection ~ 8900 2200
Wire Wire Line
	8900 2050 8900 2200
Wire Wire Line
	9300 2000 9300 2100
Connection ~ 8900 1700
Wire Wire Line
	8900 1750 8900 1700
Wire Wire Line
	8900 2200 8400 2200
Wire Wire Line
	9700 2200 8900 2200
Wire Wire Line
	8900 1700 8500 1700
Wire Wire Line
	9600 2000 9600 1700
Wire Wire Line
	9700 2000 9600 2000
Connection ~ 9300 2100
Wire Wire Line
	9350 1400 9700 1400
Wire Wire Line
	9350 1550 9350 1400
Wire Wire Line
	9200 1550 9350 1550
Wire Wire Line
	8800 1550 8900 1550
Wire Wire Line
	9300 2100 9700 2100
Wire Wire Line
	8200 2100 9300 2100
Wire Wire Line
	7100 800  7100 900 
Connection ~ 7100 800 
Wire Wire Line
	7100 800  7200 800 
Connection ~ 7100 900 
Wire Wire Line
	7100 900  7200 900 
Wire Wire Line
	7100 1200 7100 1300
Connection ~ 7100 1200
Wire Wire Line
	7100 1200 7200 1200
Connection ~ 7100 1300
Wire Wire Line
	7100 1300 7200 1300
$EndSCHEMATC

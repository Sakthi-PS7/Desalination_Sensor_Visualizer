# Project Title: Desalination_Sensor_Visualizer

# Overview
This is an embedded prototype inspired by the portable desalination units developed at NONA Technologies. I built this to emulate the sensing subsystems found in professional water quality devices, specifically focusing on how temperature and conductivity relate to salinity.

# Components Used
1. Microcontroller: ESP32 Dev Module
2. 10kΩ NTC Thermistor (B=3950)
3. Copper probes (for conductivity)
4. WS2812B LED Strip (60 LEDs)
5. Resistors: 10kΩ and 4.7kΩ for the voltage dividers.

# Working
The system uses an ESP32 to measure water quality and visualizes the "salty" content in real-time on an LED strip.

Sensing: It reads water temperature (via an NTC thermistor) and electrical conductivity (via copper probes).

Processing: Since warm water conducts electricity better than cold water, the code applies temperature compensation to normalize the readings to 25°C. This ensures the "salinity index" is based on actual mineral content, not just heat.

Visualization: A WS2812B LED strip acts as a dynamic bar graph:

    -Blue/Green: Low conductivity (Fresh/Drinking water)
    -Yellow/Orange: Medium conductivity (Hard/Brackish water)
    -Red: High conductivity (Saline/Salty)


# Takeaway
This Project gave me good kickstart on how crucial is the software (firmware) testing part is for these devices and I am seeking to leverage this hands-on experience with embedded C++, ADC sensor interfacing, and real hardware testing to accelerate firmware development for NONA’s upcoming product launch.
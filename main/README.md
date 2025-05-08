# Current Meter

This project is designed to measure and monitor electrical current using a microcontroller and sensors.

## Features
- Real-time current measurement
- Data logging capabilities
- User-friendly interface

## Installation
1. Clone the repository:
    ```bash
    git clone https://github.com/your-username/current-meter.git
    ```
2. Navigate to the project directory:
    ```bash
    cd current-meter
    ```
3. Follow the setup instructions in the `docs/SETUP.md` file.

## Usage
Run the main script to start the current meter:
```bash
python main.py
```

## Contributing
Contributions are welcome! Please see the `CONTRIBUTING.md` file for guidelines.

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

┌───────────────────────┐       ┌───────────────────────┐
│       ESP32           │       │      INA226           │
│                       │       │                       │
│  3V3  ────────────────┼───────┼─> VCC (3.3V)          │
│  GND  ────────────────┼───────┼─> GND                 │
│  GPIO22 (SCL) ────────┼───────┼─> SCL                 │
│  GPIO21 (SDA) ────────┼───────┼─> SDA                 │
│                       │       │                       │
└───────────────────────┘       └───────────┬───────────┘
                                            │
                                            ▼
┌───────────────────────┐       ┌───────────────────────┐
│  DC Power Source      │       │      Load             │
│  (e.g., Battery)      │       │  (Motor/Device)       │
│                       │       │                       │
│  (+) ──────────────── ┼───────┼─> (+)                 │
│  (-) ────────┬─────── ┼───────┼─> (-)                 │
│              │        │       │                       │
└──────────────┘        │       └───────────────────────┘
                        │
                        ▼
┌───────────────────────────────────────────────┐
│  Shunt Resistor (0.1Ω, 1%, 5W)                │
│  Placed between (-) of source and INA226 IN-  │
└───────────────────────────────────────────────┘

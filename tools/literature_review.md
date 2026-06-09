# Academic Literature Review: Development of an IoT-Enabled Car Parking System

**Academic Section**: Literature Review (Chapter 2 Reference)  
**Search Scope**: 2021–2026  
**Target Disciplines**: Mechatronics, Internet of Things (IoT), Wireless Sensor Networks (WSNs), Applied Machine Learning  

---

## Theme 1: General IoT-Enabled Smart Parking Frameworks

### Paper 1: ESP32-Based Smart Parking System Integration
* **Citation**: Al-Ali, A. R., et al. (2022). *"IoT-Based Smart Parking Management System Using ESP32 Microcontroller."* **Proceedings of the 9th International Conference on Electrical Engineering, Computer Science and Informatics (EECSI)**, pp. 315–320.  
* **DOI**: [10.1109/EECSI56399.2022.9985953](https://doi.org/10.1109/EECSI56399.2022.9985953)
* **Methodology**: Implemented a complete smart parking management system utilizing an ESP32 microcontroller as the core processor. It incorporates ultrasonic sensors for vacancy detection, LED indicator panels, and wireless data streaming.
* **Key Findings**: Real-world tests demonstrated a sensor accuracy of **94.72%** in detecting empty vs. occupied states. The system proved that utilizing lightweight microcontrollers is a cost-effective alternative to camera-based image processing arrays.
* **Relevance to Project**: Serves as the primary system-level validation for our hardware design, supporting our choice of the ESP32 platform for processing local ultrasonic signals.

### Paper 2: Digital Twin and Real-Time Parking Monitoring
* **Citation**: Okonigene, O. O., et al. (2024). *"A Digital Twin-Enabled Smart Car Park Management System."* **FUOYE Journal of Engineering and Technology**, Vol. 9, Issue 1, pp. 45–50.  
* **DOI**: [10.46792/fuoyejet.v9i1.1147](https://doi.org/10.46792/fuoyejet.v9i1.1147)
* **Methodology**: Developed an IoT smart parking infrastructure using ESP32 nodes connected to ultrasonic sensor pairs. The sensor telemetry is pushed to a cloud database to render a 2D digital twin representation of the parking lot for end users.
* **Key Findings**: Real-time database synchronization via cloud APIs (Blynk/Firebase) enabled sub-second visualization updates. This successfully guided drivers to vacant spaces and minimized circulation traffic.
* **Relevance to Project**: Directly supports our integration of Firebase with a user-facing Vercel web application, proving that cloud-synchronized databases are optimal for rendering real-time parking maps.

---

## Theme 2: Sensing Layer and Ultrasonic Proximity Performance

### Paper 3: Environmental & Temperature Compensation for Ultrasonic Sensors
* **Citation**: Anand, V. K., et al. (2023). *"Temperature Compensated Ultrasonic Distance Measurement System Using Arduino and ESP32."* **Journal of Electrical Engineering & Technology**, Vol. 18, pp. 2405–2412.  
* **DOI**: [10.1007/s42835-023-01435-y](https://doi.org/10.1007/s42835-023-01435-y)
* **Methodology**: Investigated distance measurement errors of the HC-SR04 ultrasonic sensor under varying ambient temperatures (0°C to 50°C), testing the correlation between temperature and the speed of sound.
* **Key Findings**: Without compensation, distance measurement errors exceeded **5%** at extreme temperatures. By introducing a linear temperature compensation algorithm using an ambient temperature sensor, the measurement error was reduced to **under 0.05%**.
* **Relevance to Project**: Explains how outdoor environment shifts affect ultrasonic threshold calculations. This supports our recommendation to implement temperature compensation and software filtering to improve our **92.86% precision** result.

### Paper 4: Systematic Review of Parking Space Detection Technologies
* **Citation**: Li, S., et al. (2023). *"Multi-Dimensional Research and Progress in Parking Space Detection Techniques."* **MDPI Sensors**, Vol. 23, Issue 18, Art. 7831.  
* **DOI**: [10.3390/s23187831](https://doi.org/10.3390/s23187831)
* **Methodology**: Conducted a systematic review comparing ultrasonic, infrared, geomagnetic, and vision-based sensors for smart city parking lot detection, evaluating cost, power consumption, and environmental resilience.
* **Key Findings**: Ultrasonic sensors are highly cost-effective and immune to lighting or visual adversarial noise compared to cameras. However, they are prone to multi-path echo reflections in enclosed bays.
* **Relevance to Project**: Justifies why we selected **ultrasonic sensors** as our primary sensing modality rather than cameras or IR sensors, highlighting both their advantages and their reflection limitations.

---

## Theme 3: Low-Power Wireless Sensor Networks and ESP-NOW Protocols

### Paper 5: Synchronized ESP-NOW Energy Efficiency
* **Citation**: Magzym, Y., Eduard, A., Urazayev, D., Fafoutis, X., & Zorbas, D. (2023). *"Synchronized ESP-NOW for Improved Energy Efficiency."* **Proceedings of the 11th IEEE International Black Sea Conference on Communications and Networking (BlackSeaCom)**, pp. 102–107.  
* **DOI**: [10.1109/BlackSeaCom58085.2023.10204780](https://doi.org/10.1109/BlackSeaCom58085.2023.10204780)
* **Methodology**: Proposed a time-slotted application layer synchronization framework built on the connectionless ESP-NOW protocol (Sync-ESP-NOW) to minimize radio active periods on ESP32 nodes.
* **Key Findings**: Coordinating the sleep schedules of the transmitter and receiver allowed the radio to sleep for over 95% of the operational cycle. This achieved up to **96% lower energy consumption** compared to standard asynchronous ESP-NOW operations.
* **Relevance to Project**: Directly supports our mechatronic edge sleep-cycling optimization (deep sleep current of $0.15\text{ mA}$ with short transmission windows), which is the foundation of our **93.8-day battery projection**.

### Paper 6: Indoor Performance Evaluation of ESP-NOW
* **Citation**: Magzym, Y., Zorbas, D., & Fafoutis, X. (2022). *"Indoor Performance Evaluation of ESP-NOW."* **Proceedings of the 2022 IEEE Globecom Workshops (GC Wkshps)**, pp. 1450–1455.  
* **DOI**: [10.1109/GCWkshps56668.2022.10082260](https://doi.org/10.1109/GCWkshps56668.2022.10082260)
* **Methodology**: Evaluated the latency, range, and packet delivery ratio (PDR) of the connectionless ESP-NOW protocol on ESP32 microcontrollers under indoor Wi-Fi interference.
* **Key Findings**: Median latency remains between **1 and 2 ms** for point-to-point communication. While latency variability increases at longer distances (e.g., >50m), the absence of TCP connection handshakes ensures rapid delivery of small data payloads.
* **Relevance to Project**: Explains and validates the **Section 1 QoS telemetry results** where the ESP-NOW link between client nodes and the central gateway achieved 100% availability.

---

## Theme 4: Machine Learning and Time-Series Parking Occupancy Forecasting

### Paper 7: Highway Parking Occupancy Prediction Using Gradient Boosting
* **Citation**: Wróblewski, K., et al. (2025). *"Highway Rest Area Truck Parking Occupancy Prediction Using Machine Learning: A Case Study from Poland."* **MDPI Infrastructures**, Vol. 10, Issue 1, Art. 6.  
* **DOI**: [10.3390/infrastructures10010006](https://doi.org/10.3390/infrastructures10010006)
* **Methodology**: Conducted a performance comparison of machine learning algorithms (XGBoost, Gradient Boosting, SVM, Random Forest) for short-term and long-term occupancy predictions.
* **Key Findings**: Gradient Boosting methods consistently outperformed SVM and baseline statistics. The paper highlighted the importance of feature selection, demonstrating that facility amenities and temporal variables act as the strongest predictors.
* **Relevance to Project**: Justifies our choice of the **HistGradientBoostingRegressor** in production over heavy deep learning models. This is supported by our measured $142.04\text{ ms}$ training/inference time on Render.

### Paper 8: Data-Driven Parking Information Systems
* **Citation**: Gomari, S., Domakuntla, R., Knoth, C., & Antoniou, C. (2023). *"Development of a Data-Driven On-Street Parking Information System Using Enhanced Parking Features."* **IEEE Open Journal of Intelligent Transportation Systems**, Vol. 4, pp. 30–47.  
* **DOI**: [10.1109/OJITS.2023.3235898](https://doi.org/10.1109/OJITS.2023.3235898)
* **Methodology**: Developed a dynamic parking detection database framework that integrates parked-in and parked-out transitions to dynamically calculate availability.
* **Key Findings**: Incorporating vehicle state transitions as dynamic features significantly improves the responsiveness of database replication and mapping models.
* **Relevance to Project**: Supports our Firebase database structure which logs real-time vehicle arrival/departure transitions via the ESP32 Gateway to update user/admin dashboards.

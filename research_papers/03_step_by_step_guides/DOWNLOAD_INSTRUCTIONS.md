# 🎯 03_step_by_step_guides
กลุ่ม: Paper ที่มี step-by-step guide ครบ — ใกล้เคียง project ของคุณที่สุด
ข้อกำหนด: IMU + strain gauge เท่านั้น, mid-span half-bridge บน inner face ของ left crank

---

## Paper 1 — Blommaert et al. 2025 ⭐⭐⭐⭐⭐ (PRIORITY #1 — อ่านก่อนทุก paper)
**ชื่อ:** Experimental physics with a homemade cycling power meter: Development and characterization
**ทำไมอยู่ที่นี่:** เป็น paper เดียวที่ match โปรเจคของคุณ 100% — ใช้ IMU + strain gauge เท่านั้น, half-bridge บน left crank inner face, HX711, Arduino Nano 33 BLE, BLE CPS profile และมี step-by-step สำหรับนักศึกษา
**สิ่งที่ได้:**
  - ขั้นตอนติด strain gauge ระดับ student lab
  - วิธี calibrate ด้วย known weights
  - การใช้ on-board IMU (LSM-class gyro) วัด cadence และ ω
  - validate กับ commercial power meter
**DOI:** 10.1119/5.0270674
**ลิงก์:**
- HAL (open access — FREE): https://hal.science/hal-05125140v1
- AIP Publishing (อาจต้อง subscription): https://pubs.aip.org/aapt/ajp/article/93/7/589/3349930
- ResearchGate: https://www.researchgate.net/publication/393244965
**วิธีโหลด:** ไปที่ลิงก์ HAL → กด "Télécharger le fichier" (Download file) — ฟรีทันที
**บันทึกเป็น:** `Blommaert_2025_Homemade_PowerMeter_PRIORITY.pdf`

---

## Paper 2 — Analog Devices / Cassidy 2021 ⭐⭐⭐⭐⭐ (PRIORITY #2 — engineering reference)
**ชื่อ:** How to Design a Low Power Highly Accurate Bicycle Power Meter
**ทำไมอยู่ที่นี่:** ให้ engineering-level guide ครบ รวมถึง IMU role ใน power calculation และ วิธีหา ω จาก strain periodicity เมื่อ IMU ปิด
**สิ่งที่ได้:**
  - Half-bridge placement บน left crank mid-span
  - Full signal chain: bridge → in-amp → ADC → MCU → BLE
  - IMU integration สำหรับ cadence และ crank angle
  - Power budget ละเอียด (760 µA @ 3V)
**ลิงก์ (free):**
- https://www.analog.com/en/resources/analog-dialogue/articles/low-power-highly-accurate-bicycle-power-meter.html
**วิธีโหลด:** Ctrl+P → Save as PDF
**บันทึกเป็น:** `AnalogDevices_2021_PowerMeter_Design.pdf`

---

## Paper 3 — Martín-Sosa et al. 2021 ⭐⭐⭐⭐ (IMU drift correction reference)
**ชื่อ:** Design and Validation of a Device Attached to a Conventional Bicycle to Measure the Three-Dimensional Forces Applied to a Pedal
**ทำไมอยู่ที่นี่:** ใช้ MPU6050 + strain gauge + Arduino Nano stack เหมือนกับที่คุณจะทำ — ระบุวิธีแก้ IMU gyro drift ด้วย Hall-effect sensor reset ทุก 1 รอบ ซึ่งเป็นปัญหาสำคัญที่ต้องแก้
**สิ่งที่ได้:**
  - MPU6050 I²C wiring กับ Arduino Nano
  - วิธี reset cumulative gyro drift ทุก revolution
  - microdeformation tables ต่อ 98.1N reference load
**ลิงก์ (free full text):**
- https://pmc.ncbi.nlm.nih.gov/articles/PMC8272058/
- https://www.mdpi.com/1424-8220/21/13/4590
**วิธีโหลด:** กด "Download PDF" ที่ MDPI หรือ PMC
**บันทึกเป็น:** `Martin-Sosa_2021_Pedal_IMU_StrainGauge.pdf`

---

## ลำดับการอ่านที่แนะนำ
1. **Blommaert 2025** — อ่านก่อน: เข้าใจ architecture ทั้งหมด + placement + circuit
2. **Analog Devices 2021** — อ่านต่อ: เจาะ circuit detail + part selection + power budget
3. **Martín-Sosa 2021** — อ่านสุดท้าย: แก้ปัญหา IMU drift ที่จะเจอตอน implement

## สิ่งที่ต้องทำก่อน implement
- [ ] โหลด Blommaert 2025 จาก HAL (ฟรี)
- [ ] พิมพ์ Analog Devices article บันทึกเป็น PDF
- [ ] โหลด Martín-Sosa 2021 จาก PMC (ฟรี)
- [ ] อ่านทั้ง 3 ก่อนซื้อ components

# ⚡ 02_circuit_design
กลุ่ม: การออกแบบวงจรไฟฟ้า (signal conditioning, ADC, MCU, BLE)

---

## Paper 1 — Analog Devices / Cassidy 2021 ⭐⭐⭐⭐⭐ (most complete)
**ชื่อ:** How to Design a Low Power Highly Accurate Bicycle Power Meter
**ทำไมอยู่ที่นี่:** reference design ที่ครบที่สุด — มี part numbers, schematic, power budget ครบ
**สิ่งที่ได้:** zero-drift in-amp → MAX11108 SAR ADC → MAX32666 ARM+BLE, 760 µA @ 3V, ~296h บน CR2032
**ลิงก์ (free):**
- https://www.analog.com/en/resources/analog-dialogue/articles/low-power-highly-accurate-bicycle-power-meter.html
**วิธีโหลด:** กด Ctrl+P → Save as PDF หรือใช้ browser print-to-PDF
**บันทึกเป็น:** `AnalogDevices_2021_PowerMeter_Design.pdf`

---

## Paper 2 — Pigatto et al. 2016 ⭐⭐⭐⭐
**ชื่อ:** A new crank arm based load cell, with built-in conditioning circuit and strain gages, to measure the components of the force applied by a cyclist
**ทำไมอยู่ที่นี่:** แสดงวิธี embed วงจร SMD conditioning ไว้ในช่องที่ machined ภายใน crank arm เอง
**สิ่งที่ได้:** on-arm PCB, on-chip DAC, linearity <0.6%, natural freq >136Hz
**ลิงก์:**
- https://ieeexplore.ieee.org/abstract/document/7591113  (ต้อง IEEE account)
- https://pubmed.ncbi.nlm.nih.gov/28268718/
- https://www.researchgate.net/publication/309340598  (request full-text)
**วิธีโหลด:** IEEE Xplore — log in ด้วย institutional access หรือ request ที่ ResearchGate
**บันทึกเป็น:** `Pigatto_2016_BuiltIn_Conditioning_Circuit.pdf`

---

## Paper 3 — Balbinot et al. 2014 ⭐⭐⭐⭐
**ชื่อ:** A New Crank Arm-Based Load Cell for the 3D Analysis of the Force Applied by a Cyclist
**ทำไมอยู่ที่นี่:** อธิบาย custom SMD PCB ติดตรง crank structure, Wheatstone bridge 3 แกน, Bluetooth + SD logging
**หมายเหตุ:** paper เดียวกับ folder 01 แต่ดูในมุม circuit design
**ลิงก์ (free full text):**
- https://pmc.ncbi.nlm.nih.gov/articles/PMC4299046/
**บันทึกเป็น:** `Balbinot_2014_CrankArm_LoadCell.pdf` (ถ้ามีอยู่แล้วใน folder อื่น ไม่ต้องโหลดซ้ำ)

---

## Paper 4 — Cornell ECE 4760 (Eggers & Carroll 2014)
**ชื่อ:** Bike Dash — Crank-mounted Power Meter
**ทำไมอยู่ที่นี่:** published schematic ชัดเจน, อธิบาย in-amp gain ≈200 และ PIC32 pipeline — ดีสำหรับ learning circuit fundamentals
**ลิงก์ (free):**
- https://people.ece.cornell.edu/land/courses/ece4760/FinalProjects/f2014/mje56_bwc65/mje56_bwc65/
**วิธีโหลด:** Ctrl+P → Save as PDF (รวม schematic images)
**บันทึกเป็น:** `Cornell_ECE4760_2014_BikeDash.pdf`

---

## Components ที่ควรรู้จากกลุ่มนี้
| ชิ้นส่วน | ตัวเลือก budget | ตัวเลือก precision |
|---|---|---|
| ADC/In-amp | HX711 (24-bit, $1) | MAX11108 + zero-drift in-amp |
| MCU | Arduino Nano 33 BLE | MAX32666 Cortex-M4 |
| Bridge config | Half-bridge + 2×R dummy | Full-bridge 4 gauges |
| Protocol | BLE CPS profile | ANT+ / proprietary |

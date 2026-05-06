# Draft Slides: FRA503 Bicycle Power Meter

## Slide 1: Introduction

**Title**

ระบบวัดกำลังปั่นจักรยานแบบ Add-on บน Crank Arm เดิมด้วย ESP32, Bluetooth และ Cloud Backup

**Key points**

- จักรยานเป็นพาหนะที่ไม่ก่อมลพิษ มีความคล่องตัว และถูกใช้งานได้หลายรูปแบบ
- ผู้ใช้จักรยานมีทั้งกลุ่มที่ใช้เป็นพาหนะ กลุ่มออกกำลังกาย และกลุ่มฝึกซ้อม/แข่งขัน
- กลุ่มที่เน้นสมรรถนะมักต้องการข้อมูลเชิงลึกมากขึ้น เช่น cadence และ power
- โครงงานนี้จึงมุ่งพัฒนาระบบวัดกำลังปั่นแบบต้นทุนต่ำที่เข้าถึงได้ง่ายขึ้น

**Speaker note**

จักรยานไม่ได้ถูกใช้แค่เพื่อการแข่งขัน แต่ยังใช้เป็นพาหนะและใช้เพื่อออกกำลังกายด้วย อย่างไรก็ตามกลุ่มที่เน้นสมรรถนะจะต้องการข้อมูลเชิงปริมาณมากกว่า เช่น cadence และ power ซึ่งเป็นที่มาของโครงงานนี้

**Reference**

- [1] <https://imateapot.dev/homemade-power-meter/>
- [2] <https://www.youtube.com/watch?v=CRa9djnZRu0&t=11s>

---

## Slide 2: Background Problem

**Main message**

นักปั่นมักติดตามหลายค่าระหว่างการปั่น แต่ค่าที่สะท้อน effort ได้ดีอย่าง `power` ยังเข้าถึงได้ยากกว่าค่าทั่วไป

**Suggested content**

- ค่าที่นักปั่นทั่วไปมักดู: speed, distance, time, calories
- ค่าที่กลุ่มออกกำลังกาย/สมรรถนะมักดูเพิ่ม: heart rate, cadence, power
- ในกลุ่มข้อมูลเหล่านี้ `power` เป็นตัวชี้วัดที่สำคัญ แต่ power meter เชิงพาณิชย์ยังมีราคาสูง
- จึงเกิดโจทย์ของระบบต้นทุนต่ำที่สามารถติดตั้งบนจักรยานเดิมได้

**Speaker note**

ย้ำ pain point สองชั้น คือ `cost barrier` และ `lack of openness` เมื่อเทียบกับการเรียนรู้เชิงวิศวกรรม

---

## Slide 3: Objective and Scope

**Objectives**

- วัด deformation บน crank arm ด้วย `strain gauge`
- ใช้ `IMU` เพื่อประมาณ cadence และ angular velocity
- คำนวณ estimated power จากข้อมูล 2 sensor
- ส่งข้อมูลจาก ESP32 ไปยัง smartphone ผ่าน BLE
- แสดงผล วิเคราะห์ และ backup ride data ผ่าน mobile app และ cloud

**Scope**

- In scope: `strain gauge + IMU`, BLE, mobile dashboard, calibration UI, cloud backup, security baseline
- Out of scope: commercial-grade accuracy, dual-sided power, ANT+ เต็มรูปแบบ, enclosure ระดับผลิตภัณฑ์จริง
- ในข้อเสนอฉบับนี้ใช้ `single-sided crank arm based system`
- smartphone ถูกตีความเป็นอุปกรณ์ปลายทางตัวที่สองของระบบ

**Speaker note**

ถ้าคณะกรรมการถามเรื่อง requirement 2 devices ให้ตอบตรง ๆ ว่า phase นี้ใช้ `ESP32 + smartphone` และ phase ถัดไปสามารถเพิ่ม handlebar alert node ได้

---

## Slide 4: System Diagram

**Visual**

ใช้แผนภาพจากไฟล์ `bicycle_power_meter_system_diagram.mmd`

**Talking points**

- Device layer: strain gauge, IMU, signal conditioning, ESP32, battery
- Communication layer: BLE GATT ระหว่าง ESP32 กับมือถือ และ HTTPS/Firebase SDK ระหว่างมือถือกับ cloud
- Service layer: calibration, filtering, power estimation, confidence analysis, cache/retry
- Application layer: Flutter app สำหรับ live dashboard, history, settings, notification
- Cloud layer: Firebase Firestore, optional Cloud Functions, security rules

**Key message**

ระบบนี้ออกแบบให้ mobile app เป็นทั้ง UI และ gateway สู่ cloud จึงลดความซับซ้อนของฝั่งจักรยาน

---

## Slide 5: Alternative Solution

**Products in market**

| สินค้า | ประเภท | จุดเด่น | ข้อจำกัด | ราคาอ้างอิง |
| --- | --- | --- | --- | ---: |
| Stages Gen 3 Ultegra R8000 | Left crank arm | ติดตั้งง่าย, single-sided, Accuracy ±1.5% | วัดขาเดียว | 13,100 บาท |
| 4iiii Precision 3+ Left Arm Only | Left crank arm | เบา, Accuracy ±1%, ANT+/BLE, battery life สูง | ต้องเลือก arm ให้ตรงรุ่นขาจาน | 14,000-19,354 บาท |
| Shimano FC-R9200-P Left Hand Arm | Left crank arm unit | ecosystem ของ Shimano, commercial-grade | ราคาสูงและเป็นชิ้นส่วนเฉพาะรุ่น | 19,040 บาท |

**Price chart**

![Crank arm power meter price chart](crank_power_meter_price_histogram.png)

**Interpretation**

- กราฟนี้คัดเฉพาะ `crank arm / left crank arm` เท่านั้น
- ช่วงราคาที่พบส่วนใหญ่อยู่ราว `11,800-19,040 บาท`
- มีบางรายการนำเข้าที่สูงขึ้นไปถึงประมาณ `25,783 บาท`

**Project positioning**

โปรเจกต์ของเราต่างจากสินค้าในตลาดตรงที่เป็น `add-on sensing system` ที่ติดตั้งบน crank arm เดิม โดยไม่ต้องเปลี่ยนโครงสร้างหลักของจักรยาน แต่ต้องมีการติดตั้งเซนเซอร์และ calibration อย่างเหมาะสมเพื่อให้วัดค่าได้ถูกต้อง

**Why crank arm based**

- ตรงกับแนวคิด add-on system เพราะติดตั้งบน crank arm เดิมได้
- ต้นทุนต่ำกว่า spider based และ crankset based
- เหมาะกับการติดตั้ง strain gauge และสร้าง prototype
- ซับซ้อนน้อยกว่า pedal based ในเชิงกลและการประกอบ

**References**

- Stages official: <https://stagescycling.com/en_global/product/power-meters/single-sided>
- Shimano official: <https://bike.shimano.com/pl-PL/products/components/pdp.P-FC-R9200-P.html>
- 4iiii retailer pages: <https://ccache.cc/products/4iiii-precision-3-power-meter-left-arm-only-105-r7100>, <https://ccache.cc/products/4iiii-precision-3-power-meter-left-arm-only-ultegra-r8100>, <https://ccache.cc/products/4iiii-precision-3-power-meter-left-arm-only-dura-ace-r9200>
- Thai-accessible listings: <https://www.priceza.com/s/%E0%B8%A3%E0%B8%B2%E0%B8%84%E0%B8%B2/%E0%B8%82%E0%B8%B2%E0%B8%A7%E0%B8%B1%E0%B8%95%E0%B8%95%E0%B9%8Cstages-ultegra-r8000>, <https://www.priceza.com/s/%E0%B8%A3%E0%B8%B2%E0%B8%84%E0%B8%B2/%E0%B8%82%E0%B8%B2%E0%B8%88%E0%B8%B2%E0%B8%99105>, <https://www.ubuy.co.th/th/product/IEUP1SR62-4iiii-precision-3-powermeter-ride-ready-left-side-ant-performance-meter-for-outdoor-indoor-cycling-measures-watts-cadence-calories>

---

## Slide 6: Technical Feasibility

| ด้านที่ประเมิน | ทำได้เพราะ | ความเสี่ยงหลัก | วิธีลดความเสี่ยง |
| --- | --- | --- | --- |
| Sensing on crank arm | มี precedent จากงาน DIY วัด deformation บน crank arm [1] | ติดตั้ง sensor ไม่ repeatable | ใช้ jig, zero-offset calibration, ตำแหน่งติดตั้งมาตรฐาน |
| IMU-based cadence estimation | IMU สามารถใช้ประมาณ angular velocity และ cadence จากการหมุนของ crank arm ได้ | noise และ drift | ใช้ filtering และวิเคราะห์ periodic motion |
| ESP32 BLE transmission | ESP-IDF รองรับ BLE และ GATT server อย่างเป็นทางการ [3][4] | connection drop, latency | ออกแบบ packet เล็ก, reconnect, cache ฝั่งมือถือ |
| Mobile app integration | Flutter + FlutterFire ใช้กับ Firebase ได้ตรง [5] | permission และ platform differences | เริ่ม Android-first และ lock scope |
| Cloud backup reliability | Firestore sync ข้อมูลและรองรับ offline persistence [6][7] | conflict, quota, cost | schema ง่าย, per-user path, batch sync |
| Decision support (1B) | ใช้ strain + IMU สร้าง confidence score, cadence stability, recommendation | ถ้าเป็นแค่ if-else จะยังไม่พอ | ใช้ filtering, moving average, variance และ confidence scoring |
| Calibration / accuracy | commercial meter ใช้ compensation และ calibration logic คล้ายกัน [9][10] | prototype แม่นยำต่ำกว่า commercial | ตั้งเป้าเป็น prototype และเทียบ sanity check กับ reference meter |

**Takeaway**

โครงงานนี้มีความเป็นไปได้เชิงเทคนิคในระดับ `prototype สำหรับการเรียนและพิสูจน์แนวคิด` โดยใช้ sensor เพียง 2 ชนิด คือ `strain gauge + IMU` และใช้แอปเป็น `decision support system`

**Power estimation concept**

- `Strain gauge -> torque estimate`
- `IMU -> angular velocity / cadence`
- `Estimated Power = Torque Estimate x Angular Velocity`

**Decision support (Requirement 1B)**

- แอปไม่ได้แค่แสดงค่า power แต่ประเมิน `measurement confidence`
- ใช้ข้อมูลหลายตัวร่วมกัน เช่น strain noise, cadence stability, motion consistency
- สรุปเป็นคำแนะนำให้ผู้ใช้ตัดสินใจ เช่น `accept data`, `recalibrate`, `inspect mounting`

**References**

- [1] <https://imateapot.dev/homemade-power-meter/>
- [3] <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/index.html>
- [4] <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_gatts.html>
- [5] <https://firebase.google.com/docs/flutter/setup>
- [6] <https://firebase.google.com/products/firestore>
- [7] <https://firebase.google.com/docs/firestore/manage-data/enable-offline>
- [8] <https://firebase.google.com/docs/firestore/security/get-started>
- [9] <https://stagescycling.com/en_global/product/power-meters/single-sided>
- [10] <https://bike.shimano.com/pl-PL/products/components/pdp.P-FC-R9200-P.html>

---

## Slide 7: Project Cost

| รายการ | ราคาโดยประมาณ (บาท) |
| --- | ---: |
| ESP32 Dev Board | 250 |
| Strain gauge set | 350 |
| HX711 / ADC front-end | 120 |
| IMU module | 150 |
| Battery + charging/regulation | 400 |
| Enclosure + mounting hardware | 350 |
| Wiring / connectors / protoboard | 180 |
| Epoxy / waterproofing materials | 250 |
| Spare parts + calibration jig | 500 |
| Contingency | 400 |
| **รวม** | **2,950** |

**Message**

ต้นทุนของ prototype อยู่ในระดับ `หลักพันบาท` ซึ่งต่ำกว่าราคาสินค้า commercial อย่างชัดเจน และเหมาะกับเป้าหมายของรายวิชา

---

## Slide 8: Implementation Plan

| ช่วงเวลา | งานหลัก | ผลลัพธ์ |
| --- | --- | --- |
| Week 1 | requirement + competitor study + mechanical concept | problem definition + architecture |
| Week 2 | strain gauge installation + analog readout test | bench-test data |
| Week 3 | ESP32 firmware + BLE packet | sensing node prototype |
| Week 4 | Flutter UI + Firebase integration | live dashboard + cloud path |
| Week 5 | end-to-end integration + calibration | connected prototype |
| Week 6 | testing + analysis + documentation | final report + presentation |

**Closing message**

ถ้า prototype รุ่นแรกทำงานเสถียร จุดพัฒนาต่อที่สำคัญคือเพิ่มความแม่นยำของ calibration, ปรับความทนทานเชิงกล และเพิ่มอุปกรณ์ปลายทางอีกตัวเพื่อให้ architecture แข็งแรงขึ้นตาม requirement ของวิชา

<!-- Generated from report/FRA503 Project Proposal Report 65009.docx. Embedded images were intentionally omitted and replaced with labels. -->

[Figure omitted: image1.jpg from original DOCX]

รายงาน

วิชา FRA503

Technopeuneurship in IoT Industry

สถาบันวิทยาการหุ่นยนต์ภาคสนาม มหาวิทยาลัยเทคโนโลยีพระจอมเกล้าธนบุรี

โครงการ

การพัฒนาระบบจักรยานอัจฉริยะสำหรับตรวจวัดและบันทึกข้อมูลการปั่นผ่าน BLE และคลาวด์

ชื่อกลุ่ม 5

ผู้ดำเนินงาน

นายชญานิน นาเพีย รหัสนักศึกษา 65340500009

# บทที่ 1 บทนำ

## ที่มาและความสำคัญ

จักรยานเป็นพาหนะที่ไม่ก่อมลพิษ มีความคล่องตัว และสามารถใช้งานได้หลากหลายรูปแบบ ทั้งการเดินทางในชีวิตประจำวัน การออกกำลังกาย และการฝึกซ้อมเพื่อการแข่งขัน ผู้ใช้แต่ละกลุ่มจึงมีเป้าหมายในการปั่นและความต้องการข้อมูลที่แตกต่างกันออกไป โดยกลุ่มผู้ใช้จักรยานเป็นพาหนะในชีวิตประจำวันให้ความสำคัญกับความสะดวกและความคุ้มค่า กลุ่มผู้ใช้เพื่อการออกกำลังกายสนใจความหนักเบาของการปั่นและการพัฒนาสมรรถภาพร่างกาย ขณะที่กลุ่มผู้ใช้เพื่อการฝึกซ้อมและการแข่งขันต้องการข้อมูลเชิงลึกเพื่อใช้ควบคุมการซ้อมและวางแผนการใช้พลังงานบนเส้นทางจริง

ในการติดตามประสิทธิภาพการปั่น นักปั่นมักใช้ตัวชี้วัดหลายชนิด เช่น ระยะทาง เวลา ความเร็ว อัตราการเต้นหัวใจเฉลี่ย และกำลังปั่น อย่างไรก็ตาม หากต้องการประเมินความหนักเบาของการออกแรงอย่างใกล้เคียงการใช้พลังงานจริง กำลังปั่นซึ่งหมายถึงอัตราการส่งพลังงานจากผู้ปั่นผ่านขาจานเข้าสู่ระบบขับเคลื่อนของจักรยาน เป็นตัวชี้วัดที่สำคัญกว่าความเร็วหรืออัตราการเต้นหัวใจเพียงอย่างเดียว เนื่องจากความเร็วได้รับผลจากแรงลมและความลาดชันของเส้นทาง ส่วนอัตราการเต้นหัวใจขึ้นกับสภาพร่างกาย การพักผ่อน อุณหภูมิ และโภชนาการ ดังนั้นการทราบค่ากำลังปั่นจึงช่วยให้นักปั่นควบคุมการออกแรงของตนเองและวางแผนการซ้อมได้แม่นยำขึ้นแม้เครื่องวัดกำลังปั่นจะมีประโยชน์สูง แต่ผลิตภัณฑ์เชิงพาณิชย์ในตลาดยังมีราคาค่อนข้างสูง และหลายแบบมีข้อจำกัดเรื่องความเข้ากันได้กับชิ้นส่วนเดิมของจักรยาน ทำให้ผู้ใช้ทั่วไปและผู้พัฒนาต้นแบบเข้าถึงได้ไม่ง่าย

จากแนวคิดดังกล่าว โครงงานนี้จึงเสนอการพัฒนาระบบจักรยานอัจฉริยะในรูปแบบระบบเสริมที่ติดตั้งบนขาจานข้างเดียว โดยไม่ต้องเปลี่ยนโครงสร้างหลักของจักรยาน ใช้สเตรนเกจและไอเอ็มยูเป็นแหล่งข้อมูลหลัก ใช้ ESP32 เป็นหน่วยประมวลผลบนตัวจักรยานเพื่อคำนวณค่าเบื้องต้นและสื่อสารผ่าน Bluetooth Low Energy (BLE) ไปยังแอปมือถือ จากนั้นแอปมือถือจะทำหน้าที่แสดงผล บันทึกเส้นทาง และส่งข้อมูลขึ้นฐานข้อมูลบนคลาวด์ผ่านเครือข่ายมือถือ

## ศึกษาคู่แข่งที่มีขาย

โดยทั่วไป watt meter ในจักรยานสามารถแบ่งได้เป็น 4 ประเภทหลัก ได้แก่ pedal based, crank arm based, spider based และ crankset based ซึ่งแต่ละประเภทมีจุดเด่นและข้อจำกัดต่างกัน เช่น pedal based ติดตั้งง่ายแต่ขึ้นกับระบบบันไดเดิม, spider based วัดกำลังรวมได้ดีแต่ซับซ้อนด้านการติดตั้ง, และ crankset based มีความครบในตัวแต่มีราคาสูงที่สุด

ตารางต่อไปนี้สรุปข้อดีและข้อจำกัดของ watt meter แต่ละประเภทในมุมที่เกี่ยวข้องกับการพัฒนาต้นแบบ

| ประเภท | ข้อดี | ข้อจำกัด |
| --- | --- | --- |
| Pedal based | ติดตั้งและย้ายข้ามจักรยานได้ค่อนข้างง่าย, วัดแรงจากตำแหน่งที่ผู้ปั่นออกแรงโดยตรง | ต้องเลือกให้เข้ากับระบบบันไดและรองเท้า, โครงสร้างเชิงกลซับซ้อนและรับแรงกระแทกสูง |
| Crank arm based | ต้นทุนต่ำกว่า spider/crankset based, เหมาะกับแนวคิด add-on, ติดตั้ง strain gauge และพัฒนาต้นแบบได้ง่าย | ส่วนใหญ่เป็นการวัดแบบข้างเดียว, ความแม่นยำขึ้นกับการติดตั้งและการคาลิเบรต |
| Spider based | วัดกำลังรวมของระบบขาจานได้ดี, เหมาะกับการใช้งานเชิงสมรรถนะ | การติดตั้งซับซ้อน, ขึ้นกับความเข้ากันได้ของชุดขาจาน, ต้นทุนสูง |
| Crankset based | รวมระบบไว้ในชุดเดียว, ความเรียบร้อยและความพร้อมใช้งานสูง | ราคาสูงที่สุด, ต้องเปลี่ยนชุดขาจานทั้งชุด, ไม่เหมาะกับแนวคิดต้นแบบต้นทุนต่ำ |

สำหรับโครงงานนี้เลือกโฟกัส crank arm based เนื่องจากสอดคล้องกับแนวคิด add-on system ที่ติดตั้งบน crank arm เดิมได้โดยไม่ต้องเปลี่ยนชิ้นส่วนหลักของจักรยาน มีต้นทุนต่ำกว่า spider/crankset based และเหมาะกับการพัฒนาต้นแบบด้วย strain gauge + IMU + ESP32 มากที่สุด ดังนั้นตารางเปรียบเทียบด้านล่างจึงคัดข้อมูลเฉพาะ crank arm power meter หรือ left crank arm power meter เพื่อให้เทียบกับแนวทางของโครงงานได้ตรงที่สุด

[Figure omitted: image2.png from original DOCX]

จากตัวอย่าง 10 รายการที่คัดให้เหลือเฉพาะ crank arm พบว่าช่วงราคาที่พบบ่อยอยู่ราว 11,800-19,040 บาท และมีบางรุ่นนำเข้าที่สูงขึ้นไปถึง 25,783 บาท ภาพนี้ทำให้เห็นชัดว่าต่อให้เปรียบเทียบเฉพาะกลุ่ม crank arm ซึ่งเป็นกลุ่มที่ใกล้กับโครงงานที่สุด ราคาตลาดส่วยใหญ่ค่อนข้างสูง

ข้อแตกต่างของโครงงานนี้เมื่อเทียบกับสินค้าในตลาดคือ:

เป็น add-on sensing system ที่ติดตั้งบน crank arm เดิม โดยไม่ต้องเปลี่ยนโครงสร้างหลักของจักรยาน

ใช้ ESP32 + smartphone + cloud backup เป็นสถาปัตยกรรมหลัก จึงเน้นความง่ายในการทดลองและการต่อยอดข้อมูล

เปิดโอกาสให้ปรับแต่งเฟิร์มแวร์ ตรรกะของแอป และลำดับการไหลของข้อมูลได้เอง

มีข้อจำกัดด้านความแม่นยำและความทนทานมากกว่าระบบ commercial จึงต้องพึ่งการติดตั้งและ calibration ที่เหมาะสม

## วัตถุประสงค์ของผลิตภัณฑ์/สิ่งที่ผลิตภัณฑ์ทำได้ (functions)

พัฒนาต้นแบบระบบที่สามารถอ่านค่ากำลังปั่นจาก crank arm แบบ add-on โดยไม่มีการถอดเปลี่ยนชิ้นส่วนหลักของจักรยาน

พัฒนาให้ ESP32 สามารถรับข้อมูลจาก strain gauge และ IMU แล้วสื่อสารข้อมูลผ่าน BLE ไปยังแอปมือถือ

พัฒนาแอปมือถือบนระบบ Android สำหรับแสดงผลค่ากำลังปั่นและข้อมูลการปั่นแบบเรียลไทม์

พัฒนาแอปมือถือให้สามารถส่งข้อมูลขึ้น cloud database เพื่อเก็บประวัติการปั่นได้

ศึกษาความเป็นไปได้เชิงเทคนิค ข้อจำกัด และความเสี่ยงของระบบก่อนพัฒนาต้นแบบจริง

## ขอบเขตที่ผลิตภัณฑ์ทำได้

ชิ้นงานเป็น single crank arm based

โครงจักรยานที่ใช้ในการทดลองเป็น โครงจักรยานพับ รุ่น Fika Frappe

ESP32 จะถูกติดตั้งที่จักรยาน และใช้พลังงานจาก แบตเตอรี่

การประมาณ power ใช้แหล่งข้อมูลจาก strain gauge และ IMU

แอพพลิเคชันบนระบบ android

## ประโยชน์ที่คาดว่าจะได้รับ เช่นจากธุรกิจ

ได้ต้นแบบ IoT สำหรับการวัดกำลังปั่นที่ต้นทุนต่ำและต่อยอดได้

ได้สถาปัตยกรรมที่เชื่อม อุปกรณ์ -> แอปมือถือ -> คลาวด์ อย่างครบวงจร

ได้ฐานข้อมูลประวัติการปั่นที่นำไปใช้วิเคราะห์พฤติกรรมการปั่นภายหลังได้

ช่วยลดต้นทุนในการศึกษาหลักการของ power meter เทียบกับการซื้อผลิตภัณฑ์เชิงพาณิชย์โดยตรง

เหมาะเป็นฐานต่อยอดสำหรับงานวิจัย งานเรียน หรือการพัฒนาผลิตภัณฑ์ sports analytics

## 1.6 แผนดำเนินงาน

| หน้าที่ | เมษายน 2569 | เมษายน 2569 | พฤษภาคม 2569 | พฤษภาคม 2569 |
| --- | --- | --- | --- | --- |
|  | w3 | w4 | w1 | w2 |
| สั่งของ |  |  |  |  |
| พัฒนาแอพพลิเคชั่นมือถือ |  |  |  |  |
| ออกแบบการทดลอง |  |  |  |  |
| ออกแบบ วงจรไฟฟ้า + ทดสอบ |  |  |  |  |
| ออกแบบ กล่องคอนโทรล |  |  |  |  |
| ติดตั้ง strain gauge + calibrate |  |  |  |  |
| รวมระบบ |  |  |  |  |
| ทำการทดลอง และสรุปผล |  |  |  |  |
| ทำงานงานฉบับสมบูรณ์ |  |  |  |  |

# บทที่ 2 วิธีการดำเนินการ

สถาปัตยกรรมของระบบแบ่งเป็น 5 ชั้น ได้แก่ device layer, communication layer, service layer, application layer และ cloud layer โดยฝั่งจักรยานจะมีเซนเซอร์และวงจรอ่านสัญญาณติดตั้งบน crank arm เดิม ข้อมูลจาก strain gauge และ gyroscope จะถูกส่งเข้า ESP32 เพื่อทำ edge power estimation แล้วแพ็กเป็น telemetry packet ส่งผ่าน BLE ไปยังแอปมือถือ หลังจากนั้นมือถือจะรับข้อมูลกำลังปั่น ควบคู่กับการอ่านตำแหน่งจาก GPS เพื่อนำมาสร้างเส้นทาง แสดงผลบนแผนที่ และจัดเก็บเป็นชุดข้อมูลการปั่นก่อนส่งขึ้น Firebase ผ่านเครือข่าย 4G / 5G

[Figure omitted: image3.png from original DOCX]

รูปที่ ... แสดงแผนผังระบบของโครงงานนี้

## 2.1 การออกแบบฮาร์ดแวร์

## 2.2 การออกแบบทางไฟฟ้า

## 2.3 การพัฒนาเฟิร์มแวร์

## 2.4 การพัฒนาแอปพลิเคชันบนมือถือ

## 2.5 การจัดเก็บข้อมูลบนคลาวด์

# บทที่ 3 วิธีการทดสอบ ผลและวิเคราะห์การทดสอบ

## รายการการทดลองและแผนทดสอบ

ส่วนนี้เป็นแผนการทดลองสำหรับพัฒนาระบบวัดกำลังปั่นจักรยาน โดยเรียงจากการคาลิเบรตแรง ไปจนถึงการรวมระบบและตรวจสอบค่ากำลังไฟฟ้าเชิงกลที่คำนวณได้

### ภาพรวมรายการทดลอง

| หมายเลข | ชื่อการทดลอง | เป้าหมายหลัก | สถานะ |
| --- | --- | --- | --- |
| 3.1 | คาลิเบรต Strain Gauge ด้วย HX711 แบบหลายจุด (0–4 kg) | หา `zero_offset` และ `counts_per_newton` เพื่อแปลง raw ADC เป็นแรงหน่วย N ตรวจ linearity, residual และ hysteresis | กำลังดำเนินการ |
| 3.2 | ทดสอบมุมขาจานด้วย IMU | ระบุแกน gyro ของ MPU6050 ที่ตรงกับการหมุนของขาจาน และประเมิน error ของการอินทิเกรตมุม | รอทำหลัง 3.1 |
| 3.3 | ทดสอบความทนทานทางกล | ตรวจสอบว่าจุดยึดกล่องควบคุม, จุดติด strain gauge บน crank arm และสายเชื่อมต่อไม่หลุดเสียหายระหว่างปั่นจริง | รอทำหลัง 3.2 |
| 3.4 | ทดสอบระบบรวม (Full Integration Test) | ตรวจสอบว่า ESP32, BLE, mobile app และ cloud ทำงานร่วมกันได้ขณะปั่นจริง | รอทำหลัง 3.3 |

## 3.1 การทดลองคาลิเบรต HX711 แบบหลายจุดด้วยน้ำหนักอ้างอิง 0-4 kg

### วัตถุประสงค์

การทดลองนี้มีเป้าหมายเพื่อคาลิเบรตวงจร HX711 ที่ต่อกับวงจร strain gauge / load cell bridge โดยแปลงค่าดิบจาก ADC ของ HX711 ให้กลายเป็นแรงหน่วยนิวตัน (N) สำหรับนำไปใช้ในระบบวัดกำลังปั่นจักรยานต่อไป การทดลองนี้ใช้การคาลิเบรตแบบหลายจุดจากน้ำหนักอ้างอิง 0-4 kg เพื่อให้ตรวจสอบความเป็นเส้นตรงของระบบได้ดีกว่าการใช้จุดน้ำหนักเดียว โดยจุด 5 kg ถูกตัดออกจากการทดลองชุดนี้ด้วยเหตุผลด้านความปลอดภัย เพราะจักรยานมีแนวโน้มเคลื่อนที่ไปข้างหน้าเมื่อไม่มีคนจับยึด

ค่าหลักที่ต้องหาในการทดลองนี้คือ:

- `zero_raw_average` หรือ `zero_offset`: ค่า ADC เฉลี่ยตอนยังไม่มีแรงกระทำ ใช้เป็นค่าศูนย์อ้างอิง
- `counts_per_newton`: จำนวน ADC count ที่เปลี่ยนไปต่อแรง 1 N ใช้เป็นค่าคาลิเบรตหรือ calibration divisor
- `linearity`: ความเป็นเส้นตรงระหว่างแรงอ้างอิงกับค่า `delta_raw`
- `residual` หรือ error: ค่าคลาดเคลื่อนของแต่ละจุดเมื่อเทียบกับเส้น calibration
- `hysteresis`: ความต่างของค่าที่อ่านได้ระหว่างช่วงเพิ่มน้ำหนักและช่วงลดน้ำหนัก

หลังจากได้ค่า calibration แล้ว สามารถประมาณแรงจากค่า ADC ได้ด้วยสมการ:

```text
force_N = (raw_average - zero_raw_average) / counts_per_newton
```

### การจัดชุดทดลอง

#### อุปกรณ์ที่ใช้

- ESP32 NodeMCU-32S / ESP-32S Kit
- โมดูล HX711 load cell amplifier / 24-bit ADC
- วงจร strain gauge / Wheatstone bridge / load cell circuit ที่ต่อเข้ากับ HX711 แล้ว
- น้ำหนักอ้างอิง 1 kg จำนวน 2 ถุง
- น้ำหนักอ้างอิง 2 kg จำนวน 2 ถุง
- สาย jumper และไฟเลี้ยง 3.3V จาก ESP32
- Serial Monitor ที่ baud rate `115200`

#### การต่อสาย

| HX711 Pin | จุดต่อในระบบ | หน้าที่ |
| --- | --- | --- |
| `VCC` / `DVDD` | `3.3V` | ให้ระดับ logic ตรงกับ ESP32 |
| `GND` | `GND` | ground ร่วมของระบบ |
| `DOUT` | `GPIO32` | ขาส่งข้อมูลดิจิทัลจาก HX711 |
| `SCK` / `PD_SCK` | `GPIO33` | ขา clock สำหรับอ่านข้อมูลจาก HX711 |
| `E+`, `E-`, `A+`, `A-` | วงจร strain gauge / bridge circuit | ไฟเลี้ยง bridge และสัญญาณ differential input |

> หมายเหตุ: โปรเจกต์นี้ใช้ไฟเลี้ยง/logic ของ HX711 ที่ 3.3V เพื่อให้สัญญาณ digital output จาก HX711 ปลอดภัยกับ GPIO ของ ESP32 firmware กำหนด gain เป็น `64` (Channel A, gain 64) ซึ่งให้ sensitivity สูงกว่า gain 32

#### น้ำหนักอ้างอิงและระดับโหลด

ในการทดลองนี้มีน้ำหนักอ้างอิง 1 kg จำนวน 2 ถุง และ 2 kg จำนวน 2 ถุง แต่ขอบเขตการเก็บข้อมูลจริงจำกัดที่ 0-4 kg เพื่อความปลอดภัย โดยแรงอ้างอิงคำนวณจาก:

```text
known_force_N = mass_kg * g
g = 9.80665 m/s^2
```

ตารางระดับโหลดหลักที่ใช้สำหรับ calibration:

| Level | ชุดน้ำหนัก | Mass kg | Known force N | หมายเหตุ |
| --- | --- | ---: | ---: | --- |
| 0 | ไม่มีน้ำหนัก | 0 | 0.00000 | ใช้สำหรับ tare / zero |
| 1 | 1 kg | 1 | 9.80665 | จุดต่ำสุดหลัง zero |
| 2 | 2 kg | 2 | 19.61330 | ใช้ถุง 2 kg หรือ 1 kg + 1 kg เพื่อเทียบซ้ำ |
| 3 | 1 kg + 2 kg | 3 | 29.41995 | จุดกลาง |
| 4 | 2 kg + 2 kg หรือ 1 kg + 1 kg + 2 kg | 4 | 39.22660 | ใช้เทียบ repeatability ของมวลรวมเท่ากัน |

### ขั้นตอนการทดลอง

การทดลองนี้ใช้โปรแกรม 2 ส่วนทำงานร่วมกัน: ESP32 firmware ทำหน้าที่เก็บ raw sample และ Python logger ทำหน้าที่ส่งคำสั่ง ระบุ mass/phase และบันทึก CSV อัตโนมัติ

**การเตรียมก่อนเริ่ม**

1. อัพโหลด `esp32_hx711_trial_collector.ino` ลง ESP32
2. ปิด Arduino Serial Monitor ทุกหน้าต่าง (Python logger จะใช้ COM3 แทน)
3. รัน Python logger บน PC: `python hx711_calibration_logger.py`
4. Logger จะถามให้ tare → ตรวจสอบว่าไม่มีโหลดบน strain gauge แล้วกด Enter (หรือพิมพ์ `skip` ถ้า tare ไปแล้ว)

**ลำดับการเก็บข้อมูล (loading)**

5. รอค่านิ่ง แล้วพิมพ์ `loading m0 t1` → logger ส่งคำสั่ง `c` ให้ ESP32 เก็บ 100 samples อัตโนมัติ บันทึกลง `data/hx711_calibration/loading/m0/trial01/hx711_capture.csv`
6. วางน้ำหนัก 1 kg รอนิ่ง แล้วพิมพ์ `loading m1 t1`
7. วางน้ำหนัก 2 kg รอนิ่ง แล้วพิมพ์ `loading m2 t1`
8. วางน้ำหนัก 3 kg รอนิ่ง แล้วพิมพ์ `loading m3 t1`
9. วางน้ำหนัก 4 kg รอนิ่ง แล้วพิมพ์ `loading m4 t1`

**ลำดับการเก็บข้อมูล (unloading)**

10. เอาน้ำหนักออกเหลือ 3 kg รอนิ่ง แล้วพิมพ์ `unloading m3 t1`
11. ลดน้ำหนักต่อเนื่อง พิมพ์ `unloading m2 t1`, `unloading m1 t1`, `unloading m0 t1` ตามลำดับ
    (จุด 4 kg ของ unloading ใช้ข้อมูลร่วมกับ `loading m4` ไม่ต้องเก็บซ้ำ)

**ทำซ้ำและวิเคราะห์**

12. ทำซ้ำอย่างน้อย 3 รอบ โดยใช้ suffix `t2`, `t3` ในคำสั่ง และส่ง `tare` ก่อนแต่ละรอบ
13. ใช้คำสั่ง `monitor` หรือ `mon` เพื่อดู live graph ของ `raw` และ `delta_raw` ระหว่างรอค่านิ่ง
14. หลังเก็บครบ นำไฟล์ CSV ทั้งหมดจาก `data/hx711_calibration/` มาคำนวณ `raw_avg` และ `delta_raw_avg` ต่อ trial แล้ว fit เส้นตรง `delta_raw_avg vs known_force_N` เพื่อหา `counts_per_newton`
15. เปรียบเทียบค่า loading กับ unloading เพื่อประเมิน hysteresis
16. ตรวจค่า `unloading m0` ว่า `delta_raw_avg` ใกล้ 0 หรือไม่ เพื่อประเมิน zero drift

### หลักการแปลงค่า ADC เป็นแรง

ค่าที่ HX711 อ่านได้เป็น raw ADC count ยังไม่ใช่แรงโดยตรง จึงต้องหาความสัมพันธ์ระหว่าง raw count กับแรงที่รู้ค่าจริง สำหรับการคาลิเบรตแบบหลายจุด จะ fit สมการเส้นตรง:

```text
zero_raw_average = raw เฉลี่ยตอนยังไม่มีโหลด
known_force_N = mass_kg * 9.80665
delta_raw = raw_average - zero_raw_average
delta_raw = m * known_force_N + b
counts_per_newton = m
force_N = (raw_average - zero_raw_average - b) / counts_per_newton
```

ถ้า fit แล้วค่า `b` ใกล้ 0 แสดงว่าการ tare ทำงานดี และสามารถใช้สมการอย่างง่ายได้:

```text
force_N = (raw_average - zero_raw_average) / counts_per_newton
```

ถ้า `counts_per_newton` มีค่าเป็นลบ ไม่ได้แปลว่าวงจรผิดเสมอไป โดยทั่วไปหมายถึงทิศทางแรงหรือการต่อสาย bridge ทำให้ค่า ADC ลดลงเมื่อมีแรงกระทำ สูตรยังใช้ได้ตามเดิม และสามารถกลับเครื่องหมายใน firmware ภายหลังได้ หากต้องการให้ทิศทางแรงที่เลือกแสดงเป็นค่าบวก

### ผลที่ต้องบันทึกและกราฟที่ต้องพล็อต

แต่ละ trial จะได้ไฟล์ CSV 1 ไฟล์ที่มี 100 rows บันทึกลงโฟลเดอร์ `data/hx711_calibration/{phase}/m{mass}/trial{N}/hx711_capture.csv` โดยอัตโนมัติ

CSV columns ของแต่ละ trial:

| คอลัมน์ | ความหมาย |
| --- | --- |
| `sample_idx` | ลำดับ sample (1–100) |
| `time_ms` | เวลา millis() บน ESP32 |
| `raw` | raw ADC count จาก HX711 |
| `delta_raw` | `raw - zero_raw` (คำนวณโดย firmware) |
| `zero_raw` | ค่า zero ที่ tare ไว้ (คงที่ตลอด trial) |

ตารางสรุปสำหรับรายงาน (คำนวณจาก CSV หลังเก็บข้อมูล):

| run | phase | mass_kg | known_force_N | raw_avg | delta_raw_avg | measured_force_N | error_N | note |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | loading | 0 | 0.00000 |  |  |  |  | tare |
| 1 | loading | 1 | 9.80665 |  |  |  |  | 1 kg |
| 1 | loading | 2 | 19.61330 |  |  |  |  | 2 kg หรือ 1+1 kg |
| 1 | loading | 3 | 29.41995 |  |  |  |  | 1+2 kg |
| 1 | loading | 4 | 39.22660 |  |  |  |  | 2+2 kg หรือ 1+1+2 kg |
| 1 | unloading | 3 | 29.41995 |  |  |  |  | ลดโหลด |
| 1 | unloading | 2 | 19.61330 |  |  |  |  | ลดโหลด |
| 1 | unloading | 1 | 9.80665 |  |  |  |  | ลดโหลด |
| 1 | unloading | 0 | 0.00000 |  |  |  |  | ตรวจ zero drift |

> `raw_avg` และ `delta_raw_avg` คือค่าเฉลี่ยของ 100 samples จาก CSV trial นั้น; `measured_force_N` คำนวณจาก `delta_raw_avg / counts_per_newton` หลัง fit calibration แล้ว

กราฟที่ควรพล็อต:

1. **Calibration curve**
   - แกน x: `known_force_N`
   - แกน y: `delta_raw`
   - fit เส้นตรง `delta_raw = m * known_force_N + b`
   - รายงานค่า `m`, `b` และถ้าคำนวณได้ให้รายงาน `R^2`

2. **Force time series**
   - แกน x: `time_s` หรือ sample index
   - แกน y: `measured_force_N`
   - ใช้ดู noise, settling time และการกลับศูนย์หลังเอาน้ำหนักออก

3. **Residual / error plot**
   - แกน x: `known_force_N`
   - แกน y: `error_N = measured_force_N - known_force_N`
   - ใช้ดูว่าจุดไหนคลาดจากเส้น calibration มากผิดปกติ

4. **Loading vs unloading plot**
   - พล็อต `delta_raw` หรือ `measured_force_N` แยกสีระหว่าง loading และ unloading
   - ใช้ประเมิน hysteresis ของชุด strain gauge / โครงสร้างติดตั้ง

### โปรแกรมที่ใช้ในการทดลอง

การทดลองนี้ใช้โปรแกรม 2 ส่วน:

**1. ESP32 Firmware**
ไฟล์: `electrical/experiment/hx711_calibration/esp32_hx711_trial_collector/esp32_hx711_trial_collector.ino`

| คำสั่ง (Serial) | หน้าที่ |
| --- | --- |
| `h` | แสดง help |
| `s` | แสดงสถานะ tare ปัจจุบัน |
| `t` | tare ไม่มีโหลด เก็บ `zero_raw` จาก 20 samples |
| `c` | เก็บ 100 raw samples แล้วส่งออก Serial ในรูปแบบ `BEGIN_TRIAL ... END_TRIAL` |
| `r1` / `r0` | เปิด/ปิด live report รูปแบบ `LIVE,time_ms,raw,delta_raw` |

**2. Python Logger**
ไฟล์: `electrical/experiment/hx711_calibration/hx711_calibration_logger.py`

| คำสั่ง (terminal) | หน้าที่ |
| --- | --- |
| `loading m0 t1` | เก็บ trial loading mass 0 kg ครั้งที่ 1 บันทึกลง `data/.../loading/m0/trial01/` |
| `unloading m3 t1` | เก็บ trial unloading mass 3 kg ครั้งที่ 1 |
| `l m4 t2` | shortcut ของ `loading m4 t2` |
| `u m2 t1` | shortcut ของ `unloading m2 t1` |
| `monitor` / `mon` | เปิด live graph แสดง `raw` และ `delta_raw` แบบ real-time |
| `tare` | ส่งคำสั่ง tare ไปยัง ESP32 |
| `status` / `s` | ดึงสถานะ tare จาก ESP32 |
| `q` | ออกจากโปรแกรม |

Reference library: HX711 Arduino Library by bogde (https://github.com/bogde/HX711)


## 3.2 การทดลองทดสอบมุมขาจานด้วย IMU

### วัตถุประสงค์

ตรวจสอบว่าแกน gyroscope ของ MPU6050 ที่ติดตั้งบน crank arm สามารถสะท้อนทิศทางการหมุนจริงของขาจานได้ถูกต้อง และประเมิน error สะสมของการอินทิเกรตมุมจาก angular velocity (rad/s) เทียบกับมุมอ้างอิงจริง ผลของการทดลองนี้จะนำไปใช้เลือกแกนและเครื่องหมายสำหรับคำนวณ `power_W = torque_Nm × omega_rad_s` ในขั้นต่อไป

### การจัดชุดทดลอง

#### อุปกรณ์ที่ใช้

- ESP32 NodeMCU-32S + MPU6050 (SDA GPIO21, SCL GPIO22)
- Serial Monitor ที่ baud rate `115200`
- ไม้โปรแทรกเตอร์หรือเครื่องมือวัดมุมอ้างอิง
- จักรยาน Fika Frappe หรือ crank arm ที่ถอดออกมาตั้งบนโต๊ะ

#### การตั้งค่า firmware

- อัพโหลด sketch ที่พิมพ์ค่า `gyro_x`, `gyro_y`, `gyro_z` (rad/s) ออก Serial ทุก 10 ms
- ใช้ไลบรารีหรือ register อ่าน MPU6050 ตรง ๆ ผ่าน Wire เช่นเดียวกับ `mpu6050_test.ino`

### ขั้นตอนการทดลอง

1. ติดตั้ง MPU6050 บน crank arm ให้แกน gyro แกนใดแกนหนึ่งชี้ขนานกับแกนหมุนของ bottom bracket
2. วาง crank arm ที่ตำแหน่ง 0° (12 นาฬิกา) บันทึกค่าเฉลี่ย gyro ขณะหยุดนิ่ง 2 วินาที
3. หมุน crank ทีละ 90° ตามเข็มนาฬิกา รวม 4 จุดต่อรอบ (0°, 90°, 180°, 270°) แล้วกลับ 360°
4. ที่แต่ละจุด หยุดนิ่งและบันทึกค่าเฉลี่ย gyro ของทุกแกน
5. อินทิเกรตค่า `omega` (rad/s) × `dt` (s) เพื่อหามุมสะสม เทียบกับมุมอ้างอิง
6. ทำซ้ำ 3 รอบ บันทึก drift และ error รอบเต็ม

### ผลที่ต้องบันทึกและกราฟที่ต้องพล็อต

ข้อมูลดิบที่ควรบันทึก:

| run | step | angle_ref_deg | gyro_axis_rad_s | angle_integrated_deg | error_deg |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0 | 0 | | 0 | |
| 1 | 1 | 90 | | | |
| 1 | 2 | 180 | | | |
| 1 | 3 | 270 | | | |
| 1 | 4 | 360 | | | |

กราฟที่ควรพล็อต:

1. **Angle time series** — แกน x: เวลา (s), แกน y: มุมสะสม (deg) เทียบกับมุมอ้างอิง
2. **Error plot** — แกน x: มุมอ้างอิง (deg), แกน y: error (deg) แต่ละรอบ

### เกณฑ์ผ่าน

- ระบุแกน gyro ที่ตรงกับการหมุน crank ได้ชัดเจน
- |error| สะสมต่อ 1 รอบเต็ม (360°) ไม่เกิน ±10°
- ค่า gyro ขณะหยุดนิ่งอยู่ใกล้ 0 rad/s (drift ต่ำ)

---

## 3.3 การทดลองทดสอบความทนทานทางกล

### วัตถุประสงค์

ตรวจสอบว่าการยึดติดของกล่องควบคุม ESP32, จุดติด strain gauge บน crank arm และสายเชื่อมต่อทั้งหมด ไม่หลุด ไม่เสียหาย และไม่กระทบการอ่านค่าเซนเซอร์เมื่อผ่านการสั่นสะเทือนและแรงที่เกิดจากการปั่นจักรยานจริง

### การจัดชุดทดลอง

#### อุปกรณ์ที่ใช้

- ระบบ ESP32 พร้อม HX711, MPU6050, OLED และ buzzer ครบชุด ติดตั้งบนจักรยาน Fika Frappe
- OLED หรือ Serial Monitor สำหรับยืนยันค่าเซนเซอร์ตลอดการทดสอบ
- กล้องหรือโทรศัพท์สำหรับถ่ายรูปก่อน–หลัง
- นาฬิกาจับเวลา

### ขั้นตอนการทดลอง

1. ตรวจสอบและถ่ายรูปสภาพจุดยึดทั้งหมดก่อนเริ่มทดสอบ
2. เปิดระบบ ยืนยันว่า HX711 และ MPU6050 อ่านค่าได้ปกติผ่าน OLED
3. ปั่นจักรยานแบบ stationary (ยกล้อหลัง) หรือปั่นบนพื้นราบ นาน 10 นาที ด้วยความพยายามปกติ
4. ระหว่างปั่น สังเกต OLED ว่าค่าเซนเซอร์แสดงผลสม่ำเสมอ ไม่มีค่าหายหรือกระโดดผิดปกติ
5. หลังปั่นครบ 10 นาที หยุดและตรวจสภาพจุดยึด สายไฟ และจุดติด strain gauge
6. ถ่ายรูปสภาพหลังทดสอบเปรียบเทียบกับก่อน

### ผลที่ต้องบันทึก

| รายการ | ผลก่อนทดสอบ | ผลหลังทดสอบ | หมายเหตุ |
| --- | --- | --- | --- |
| จุดยึดกล่องควบคุม | | | ยึดแน่น / หลวม / หลุด |
| จุดติด strain gauge บน crank arm | | | ปกติ / หลุด / เสียหาย |
| สายเชื่อมต่อ HX711 และ MPU6050 | | | ปกติ / หลวม / หลุด |
| ค่า sensor ระหว่างปั่น | | | ต่อเนื่อง / ขาดหาย / กระโดด |

### เกณฑ์ผ่าน

- ไม่มีชิ้นส่วนใดหลุดหรือเสียหายหลังปั่น 10 นาที
- ค่า sensor อ่านได้ต่อเนื่องตลอดการทดสอบ ไม่มีการขาดหายหรือค่ากระโดดผิดปกติ

---

## 3.4 การทดลองทดสอบระบบรวม (Full Integration Test)

### วัตถุประสงค์

ตรวจสอบว่าระบบทั้งหมดทำงานร่วมกันได้ขณะปั่นจักรยานจริง ได้แก่ ESP32 รับข้อมูล HX711 และ MPU6050 แล้วคำนวณค่ากำลังปั่น (W) ส่งผ่าน BLE ไปยัง mobile app ที่แสดงผล real-time และส่งข้อมูลขึ้น Firebase ผ่านเครือข่ายมือถือ

### การจัดชุดทดลอง

#### อุปกรณ์ที่ใช้

- ระบบ ESP32 + HX711 + MPU6050 พร้อม firmware คำนวณ power และ BLE advertising
- โทรศัพท์ Android พร้อม mobile app ที่เชื่อมต่อ BLE และส่งข้อมูล Firebase
- เครือข่ายมือถือ 4G/5G
- Firebase console สำหรับยืนยันข้อมูล

#### ค่าที่ firmware ต้องส่งใน BLE packet

| ฟิลด์ | หน่วย | หมายเหตุ |
| --- | --- | --- |
| `force_N` | N | จาก HX711 หลัง calibration |
| `omega_rad_s` | rad/s | จาก MPU6050 แกนที่เลือก |
| `power_W` | W | `torque_Nm × omega_rad_s`; `torque_Nm = force_N × crank_length_m` |
| `timestamp_ms` | ms | `millis()` บน ESP32 |

### ขั้นตอนการทดลอง

1. เปิดระบบ ESP32 และยืนยัน BLE advertising ด้วย Serial Monitor
2. เปิด mobile app เชื่อมต่อ BLE กับ ESP32
3. ปั่นจักรยานบนพื้นราบนาน 5 นาที สังเกตหน้าจอ app ว่าค่า power (W) อัพเดทสม่ำเสมอ
4. บันทึก latency โดยประมาณระหว่างการเคลื่อนไหวจริงกับการอัพเดทบนหน้าจอ app
5. หลังปั่น เข้า Firebase console ตรวจว่ามีข้อมูล session บันทึกครบตลอด 5 นาที
6. บันทึกความสมบูรณ์ของข้อมูลและช่วงที่ขาดหาย (ถ้ามี)

### ผลที่ต้องบันทึก

| ตัวชี้วัด | ค่าที่ได้ | เกณฑ์ |
| --- | --- | --- |
| BLE latency (ประมาณ) | | < 500 ms |
| ค่า power ขณะปั่น (W) | | แสดงผลสม่ำเสมอ ไม่ติดค้างหรือค่าผิดปกติ |
| ข้อมูล session ใน Firebase | | ครบ ไม่มีช่วงขาดหาย |
| สถานะ BLE connection | | ไม่หลุดตลอด 5 นาที |

### เกณฑ์ผ่าน

- Mobile app รับค่า power และแสดงผล real-time ได้ตลอดการทดสอบ 5 นาที
- ข้อมูล session ขึ้น Firebase ครบและไม่มีช่วงขาดหาย
- BLE ไม่หลุดระหว่างปั่น

---

อธิบายวิธีการทดสอบ

อธิบายผล และวิเคราะห์การทดสอบ ต้นแบบ (prototype) ที่ได้พัฒนามา

# บทที่ 4 สรุปผลการดำเนินงาน และข้อเสนอแนะ การปรับปรุงต้นแบบ

# บทที่ 5 งบการเงิน

| ลำดับ | รายการ | จำนวน | ราคาต่อหน่วย | ราคา (บาท) |
| --- | --- | --- | --- | --- |
| 1 | Strain gauge | 4 | 12.5 | 50 |
| 2 | HX711 | 1 | 50 | 50 |
| 3 | MPU6050 | 1 | 90 | 90 |
| 4 | Battery 7.4v | 1 | 350 | 350 |
| 5 | PLA-plus-filament | 1 | 500 | 500 |
| 6 | Wiring / connectors /switch | 1 | 300 | 300 |
| 7 | step-down-5V | 1 | 40 | 40 |
| 8 | กาว epoxy | 1 | 120 | 120 |
|  |  |  |  |  |
| รวม |  |  |  | 1500 |

# อ้างอิง

Steve Jarvis, “Homemade Power Meter,” available: https://imateapot.dev/homemade-power-meter/ (accessed 2026-04-09).

YouTube reference provided by project owner, available: https://www.youtube.com/watch?v=CRa9djnZRu0&t=11s (accessed 2026-04-09).

Espressif, “Bluetooth API - ESP32,” available: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/index.html (accessed 2026-04-09).

Espressif, “GATT Server API - ESP32,” available: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_gatts.html (accessed 2026-04-09).

Firebase, “Add Firebase to your Flutter app,” available: https://firebase.google.com/docs/flutter/setup (accessed 2026-04-09).

Firebase, “Cloud Firestore,” available: https://firebase.google.com/products/firestore (accessed 2026-04-09).

Firebase, “Access data offline,” available: https://firebase.google.com/docs/firestore/manage-data/enable-offline (accessed 2026-04-09).

Firebase, “Get started with Cloud Firestore Security Rules,” available: https://firebase.google.com/docs/firestore/security/get-started (accessed 2026-04-09).

Stages Cycling, “Single Sided Power Meters,” available: https://stagescycling.com/en_global/product/power-meters/single-sided (accessed 2026-04-09).

Shimano, “FC-R9200-P,” available: https://bike.shimano.com/pl-PL/products/components/pdp.P-FC-R9200-P.html (accessed 2026-04-09).

CCACHE, “4iiii Precision 3+ Power Meter Left Arm Only - 105 R7100,” available: https://ccache.cc/products/4iiii-precision-3-power-meter-left-arm-only-105-r7100 (accessed 2026-04-09).

Priceza, “ขาวัตต์ Stages Ultegra R8000,” available: https://www.priceza.com/s/ราคา/ขาวัตต์stages-ultegra-r8000 (accessed 2026-04-09).

Ubuy Thailand, “4iiii Precision 3 Powermeter Ride Ready - Left-Side,” available: https://www.ubuy.co.th/th/product/IEUP1SR62-4iiii-precision-3-powermeter-ride-ready-left-side-ant-performance-meter-for-outdoor-indoor-cycling-measures-watts-cadence-calories (accessed 2026-04-09).

CCACHE, “Shimano Dura-Ace FC-R9200-P Left Hand Crank Arm Unit,” available: https://ccache.cc/products/shimano-dura-ace-fc-r9200-p-left-hand-crank-arm-powermeter (accessed 2026-04-09).

Priceza, “ขาจาน 105,” available: https://www.priceza.com/s/ราคา/ขาจาน105 (accessed 2026-04-09).

CCACHE, “4iiii Precision 3+ Power Meter Left Arm Only - GRX 810/820,” available: https://ccache.cc/products/4iiii-precision-3-power-meter-left-arm-only-grx-810-820 (accessed 2026-04-09).

CCACHE, “4iiii Precision 3+ Power Meter Left Arm Only - Ultegra R8100,” available: https://ccache.cc/products/4iiii-precision-3-power-meter-left-arm-only-ultegra-r8100 (accessed 2026-04-09).

CCACHE, “4iiii Precision 3+ Power Meter Left Arm Only - Dura-Ace R9200,” available: https://ccache.cc/products/4iiii-precision-3-power-meter-left-arm-only-dura-ace-r9200 (accessed 2026-04-09).

CCACHE, “Shimano FC-R8100-P Left Hand Crank Arm Unit 172.5mm,” available: https://ccache.cc/products/shimano-fc-r8100-p-left-hand-crank-arm-unit-172-5mm (accessed 2026-04-09).

---

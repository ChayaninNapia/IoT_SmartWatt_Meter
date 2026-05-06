# 📍 01_strain_gauge_placement
กลุ่ม: ตำแหน่งการติด strain gauge บน crank arm

---

## Paper 1 — Balbinot et al. 2014 ⭐⭐⭐⭐⭐
**ชื่อ:** A New Crank Arm-Based Load Cell for the 3D Analysis of the Force Applied by a Cyclist
**ทำไมอยู่ที่นี่:** ใช้ SolidWorks FEA หา strain peak zones ก่อนติด gauge — เป็น paper ที่ให้ FEA map ของตำแหน่งละเอียดที่สุด
**ลิงก์ (free full text):**
- https://pmc.ncbi.nlm.nih.gov/articles/PMC4299046/
- https://www.mdpi.com/1424-8220/14/12/22921
**วิธีโหลด:** กด "Download PDF" ที่มุมขวาบนของ MDPI หรือ "PDF" ที่ PubMed Central
**บันทึกเป็น:** `Balbinot_2014_CrankArm_LoadCell.pdf`

---

## Paper 2 — Gutiérrez-Moizant et al. 2020 ⭐⭐⭐⭐⭐
**ชื่อ:** Validation and Improvement of a Bicycle Crank Arm Based in Numerical Simulation and Uncertainty Quantification
**ทำไมอยู่ที่นี่:** ใช้ Abaqus FEA หา high-stress zones และติด rosette gauges ตามนั้น — มี uncertainty quantification ละเอียดที่สุดในวรรณกรรม
**ลิงก์ (free full text):**
- https://pmc.ncbi.nlm.nih.gov/articles/PMC7180959/
- https://www.mdpi.com/1424-8220/20/7/1814
**วิธีโหลด:** กด "Download PDF" ที่ MDPI หรือ PMC
**บันทึกเป็น:** `Gutierrez-Moizant_2020_FEA_Validation.pdf`

---

## Paper 3 — Design and Engineering of an Accurate Bicycle Power Meter (2014)
**ชื่อ:** Design and Engineering of an Accurate Bicycle Power Meter
**ทำไมอยู่ที่นี่:** ใช้ FE optimisation ของ NDS crank หา "neutral zone below spindle" และอธิบายชัดว่าทำไม near-BB และ near-pedal ถึงแย่
**ลิงก์:**
- https://www.researchgate.net/publication/303517617_Design_and_Engineering_of_an_Accurate_Bicycle_Power_Meter
**วิธีโหลด:** กด "Request full-text" หรือ log in ResearchGate แล้วกด "Download"
**บันทึกเป็น:** `DesignEngineering_2014_AccuratePowerMeter.pdf`

---

## จุดสำคัญที่ paper กลุ่มนี้ agree ร่วมกัน
- ✅ ติดที่ **mid-span บน inner face** ของ left crank arm
- ❌ หลีกเลี่ยง near-BB (stress concentration + mixed loading)
- ⚠  หลีกเลี่ยง near-pedal (pedal offset → axial crosstalk)

# Business Card — Philipp (KiCad)

KiCad project containing the PCB and schematic for Philipp's business card.

## Overview

This folder contains the KiCad project files for the "business card" PCB:

- schematic (.kicad_sch)
- PCB (.kicad_pcb)
- project and resource files
- footprint cache and backups

Important files:

- `business_card_philipp.kicad_pro` — KiCad project file  
- `business_card_philipp.kicad_sch` — Schematic  
- `business_card_philipp.kicad_pcb` — PCB layout  
- `business_card_philipp.kicad_prl` — Project resource list  
- `fp-info-cache` — KiCad footprint cache (generated, large)  
- `business_card_philipp-backups/` — Backups folder (may contain .zip files)

---

## Git ignore recommendations

To prevent backup files and generated cache from being committed and pushed, add one of the following to the repository `.gitignore`.

Ignore the whole backups folder and fp-info-cache (recommended):

```gitignore
# Ignore KiCad backup folder for Philipp's business card
/business_card_philipp/business_card_philipp-backups/

# Ignore KiCad footprint cache (large generated file)
/business_card_philipp/fp-info-cache

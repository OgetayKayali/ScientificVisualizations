# IceCube Neutrino Observatory

Ten scientific animations created by **Ogetay Kayali** about neutrinos, optical detection, and the IceCube Neutrino Observatory. These are part of the [Scientific Visualizations collection](../README.md).

The visuals use a consistent navy and ice-blue palette, minimal text, and widely recognized scientific symbols. They are designed for reuse with narration or subtitles in different languages. Every animation includes a silent **3840 × 2160, 30 fps H.264 MP4**, a **1920 × 1080** viewing copy, English captions, timing metadata, a poster, a short `INFO.md`, and scientific notes.

## Browse the animations

| # | Animation and info | Duration | Videos | Renderer |
|---|---|---:|---|---|
| 01 | [IceCube Detector Array](exports/01_icecube_detector_array/INFO.md) | 32 s | [4K](exports/01_icecube_detector_array/01_icecube_detector_array_4k.mp4) · [1080p](exports/01_icecube_detector_array/01_icecube_detector_array_1080p.mp4) | [render_detector_array_4k.py](animation/icecube_samples/render_detector_array_4k.py) |
| 02 | [Photomultiplier Electron Multiplication](exports/02_photomultiplier_electron_multiplication/INFO.md) | 24 s | [4K](exports/02_photomultiplier_electron_multiplication/02_photomultiplier_electron_multiplication_4k.mp4) · [1080p](exports/02_photomultiplier_electron_multiplication/02_photomultiplier_electron_multiplication_1080p.mp4) | [render_photomultiplier_electron_multiplication.py](animation/icecube_samples/render_photomultiplier_electron_multiplication.py) |
| 03 | [Solar Photons and Neutrinos](exports/03_solar_photons_and_neutrinos/INFO.md) | 34 s | [4K](exports/03_solar_photons_and_neutrinos/03_solar_photons_and_neutrinos_4k.mp4) · [1080p](exports/03_solar_photons_and_neutrinos/03_solar_photons_and_neutrinos_1080p.mp4) | [render_solar_photons_and_neutrinos.py](animation/icecube_samples/render_solar_photons_and_neutrinos.py) |
| 04 | [Electron and Muon Tracks: A Metal-Plate Analogy](exports/04_electron_and_muon_tracks_plate_analogy/INFO.md) | 82 s | [4K](exports/04_electron_and_muon_tracks_plate_analogy/04_electron_and_muon_tracks_plate_analogy_4k.mp4) · [1080p](exports/04_electron_and_muon_tracks_plate_analogy/04_electron_and_muon_tracks_plate_analogy_1080p.mp4) | [render_electron_muon_plate_analogy.py](animation/icecube_samples/render_electron_muon_plate_analogy.py) |
| 05 | [Light Speed in Vacuum and Water](exports/05_light_speed_in_vacuum_and_water/INFO.md) | 40 s | [4K](exports/05_light_speed_in_vacuum_and_water/05_light_speed_in_vacuum_and_water_4k.mp4) · [1080p](exports/05_light_speed_in_vacuum_and_water/05_light_speed_in_vacuum_and_water_1080p.mp4) | [render_light_speed_comparison.py](animation/icecube_samples/render_light_speed_comparison.py) |
| 06 | [Sonic Boom and Cherenkov Radiation](exports/06_sonic_boom_and_cherenkov_radiation/INFO.md) | 32 s | [4K](exports/06_sonic_boom_and_cherenkov_radiation/06_sonic_boom_and_cherenkov_radiation_4k.mp4) · [1080p](exports/06_sonic_boom_and_cherenkov_radiation/06_sonic_boom_and_cherenkov_radiation_1080p.mp4) | [render_cherenkov_radiation.py](animation/icecube_samples/render_cherenkov_radiation.py) |
| 07 | [Neutrino Direction, Timing, and Ice Calibration](exports/07_neutrino_direction_timing_and_ice_calibration/INFO.md) | 132 s | [4K](exports/07_neutrino_direction_timing_and_ice_calibration/07_neutrino_direction_timing_and_ice_calibration_4k.mp4) · [1080p](exports/07_neutrino_direction_timing_and_ice_calibration/07_neutrino_direction_timing_and_ice_calibration_1080p.mp4) | [render_neutrino_direction_and_ice_calibration.py](animation/icecube_samples/render_neutrino_direction_and_ice_calibration.py) |
| 08 | [IceCube Construction and Discovery History](exports/08_icecube_construction_and_discovery_history/INFO.md) | 70 s | [4K](exports/08_icecube_construction_and_discovery_history/08_icecube_construction_and_discovery_history_4k.mp4) · [1080p](exports/08_icecube_construction_and_discovery_history/08_icecube_construction_and_discovery_history_1080p.mp4) | [render_icecube_history.py](animation/icecube_samples/render_icecube_history.py) |
| 09 | [Applications of Electromagnetic Radiation](exports/09_applications_of_electromagnetic_radiation/INFO.md) | 40 s | [4K](exports/09_applications_of_electromagnetic_radiation/09_applications_of_electromagnetic_radiation_4k.mp4) · [1080p](exports/09_applications_of_electromagnetic_radiation/09_applications_of_electromagnetic_radiation_1080p.mp4) | [render_electromagnetic_radiation_applications.py](animation/icecube_samples/render_electromagnetic_radiation_applications.py) |
| 10 | [The Standard Model and Cosmic Neutrinos](exports/10_standard_model_and_cosmic_neutrinos/INFO.md) | 38 s | [4K](exports/10_standard_model_and_cosmic_neutrinos/10_standard_model_and_cosmic_neutrinos_4k.mp4) · [1080p](exports/10_standard_model_and_cosmic_neutrinos/10_standard_model_and_cosmic_neutrinos_1080p.mp4) | [render_standard_model_and_cosmic_neutrinos.py](animation/icecube_samples/render_standard_model_and_cosmic_neutrinos.py) |

![Standard Model animation](exports/10_standard_model_and_cosmic_neutrinos/poster.jpg)

## Reproduce the animations

Install Python 3.11 or later and FFmpeg. Run from this topic folder:

```sh
python -m pip install -r requirements.txt
python animation/icecube_samples/render_standard_model_and_cosmic_neutrinos.py --scale 2
```

For animations 02–10, `--scale 2` renders native 4K and also creates a 1080p viewing copy; `--scale 1` renders 1080p directly. Add `--preview` to write still previews and English timing/caption files without encoding the video. Output filenames match the published English names. Rendering overwrites the corresponding outputs in `exports/`.

Animation 01 has separate entry points:

```sh
python animation/icecube_samples/render_detector_array.py
python animation/icecube_samples/render_detector_array_4k.py
```

The first writes the original 1080p version, English captions, and cue metadata; it accepts `--preview`. The second rasterizes the same geometry natively at 4K. Keep all source files together: renderers share drawing helpers and geometry. `legacy_rendering_helpers.py` supports the final renderers and retains earlier scene functions; it is not a final-animation entry point.

FFmpeg is resolved from `FFMPEG_BINARY`, then PATH, then the original Windows fallback. Fonts are resolved from `ANIMATION_FONT` and `ANIMATION_FONT_BOLD`, or common Segoe UI, DejaVu Sans, Liberation Sans, and Arial paths. The published videos use Segoe UI on Windows; another font may slightly change symbol placement. Font files are not redistributed. Dependencies retain their own licenses.

## Files and timing

- `exports/01_.../` through `exports/10_.../`: one descriptive folder per animation, including both video resolutions.
- `INFO.md`: short explanation, asset links, and credits for that animation.
- `SCIENTIFIC_NOTES.md`: scene sequence, scientific assumptions, limitations, and references.
- `*.en.srt`: English captions, separate from the video.
- `timing.json`: English narration, visual descriptions, and editorial cue times.
- `animation/icecube_samples/`: editable Python renderers and helpers.
- `manifest.json`: asset paths, durations, original render versions, and SHA-256 video checksums.

English captions adapt the original narration for an international audience and retain the existing cue boundaries. No recorded English narration was used; retiming may be needed for a particular voice track. Scientific symbols and existing proper names remain in the videos. The MP4s preserve the approved visual renders; this English release changes filenames, supporting text, and renderer output paths.

These are schematic teaching visuals, not measured IceCube events or calibrated detector-response simulations. Read each animation's scientific notes before presenting quantitative interpretations.

## Credit, GPT disclosure, and reuse

Created and directed by **Ogetay Kayali**. GPT (OpenAI) was used as an assistant during development for code generation, revisions, documentation, and translation. The rendered images are produced procedurally by the supplied Python code; no stock video, external photographs, or synthetic narration are included.

Copyright (c) 2026 Ogetay Kayali. The animations, source code, and documentation may be used, copied, shared, and modified free of charge for **noncommercial educational purposes**, with attribution and the license notice retained. **All other uses require prior written permission.** Read the full [LICENSE.md](../LICENSE.md) and [CREDITS.md](../CREDITS.md). This is a custom, restricted-use license, not an open-source license.

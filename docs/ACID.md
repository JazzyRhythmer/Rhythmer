# Acid generation

![Rhythmer Acid generation](motion/acid-generate.gif)

Assign ACID to a track. Press EDIT repeatedly, or use SELECT, to reach the ACID tab and 303 GEN 1 / 2. OCT+ creates a NEW line; OCT− applies MUTATE. DENS, ACCENT, SLIDE and OCTS set the character of the result. The second generator page supplies further controls.

Set the root and scale in SCL first: NEW and MUTATE follow them. An untouched scale starts in minor. Each Acid track owns its sound and pattern, so multiple Acid machines can run together. Edit the generated notes in SEQ, then save the section/project.

The generator and bass engine are derived from X0X, integrated into the JIANT-based Rhythmer system. This silent preview is rendered from the firmware UI on a host, not recorded from hardware.

## First line

1. Select an Acid track with ALGORITHM (T5 initially).
2. Press SCL and set ROOT and SCALE. NEW and MUTATE always follow these settings; an untouched scale defaults to minor.
3. Press EDIT repeatedly to ACID → 303 GEN 1. Set DENS (note density), ACCENT, SLIDE and OCTS.
4. OCT+ makes a new line. OCT− mutates the current line. Press PLAY and inspect/edit its MIDI in SEQ.
5. The second generator page includes ROOT / SCALE and octave / periodic mutation controls. Set periodic mutation to OFF for a stable repeating line.
6. Save the section/project; export a musical backup in the editor.

## Choose the sound

The Acid machine has X0X ACID plus four Hortator presets: HT SQUELCH, HT DEEPBASS, HT SCREAMER and HT SQUAREAC. These are sounds of the Acid machine; their generated notes still use the same SCL and SEQ workflow. The SOUND/TONE pages expose cutoff, resonance, envelope amount, decay, accent, model, slide and drive. Ordinary synth presets with former Acid-like names were renamed to distinguish them from this machine.

For genre-based MIDI over your existing loop, use HOME + SELECT → GEN, choose the genre and explicitly target the Acid track (T5 MIDI in the default setup), then OCT+. This keeps the existing sound, tempo, globals, automation and other tracks. ALL generates only T1–T4, including macro assignment; it leaves T5/T6 notes and settings intact.

Multiple Acid tracks are allowed, but the complete six-track arrangement and shared effects determine CPU load. Shorten dense releases or reduce expensive parts if the CPU warning appears. Check MENU → ABOUT → OCT+ / PROFILE rather than assuming one engine is always responsible.

[Detailed Acid, sequencing and backup manual](https://jazzyrhythmer.github.io/Rhythmer/#guide).

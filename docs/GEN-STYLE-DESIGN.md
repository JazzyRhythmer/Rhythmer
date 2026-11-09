# Rhythmer GEN: ten additional profiles

These are original Rhythmer arrangements informed by production practices, not reproductions of an artist or a named track. Artist references guide the design; the menu uses descriptive musical names. The 59-style menu preserves every old genre ID. No French Touch or Afro House profile is added.

## Research translated into musical rules

| Display name | Tempo | Arrangement identity | Preferred sounds, with alternatives still eligible |
|---|---:|---|---|
| BOUNCE HOUSE | 108–120 | Soulful swung house/disco; syncopated bass, late offbeat hats, compact extended chords and two-bar kick answers | Analog/Lo-Fi/SID bass; Phase/Wheel/Wavetable keys; warm pluck or a short lead hook |
| 90S HOUSE | 120–128 | Firm quarter-note kick, house hats, offbeat bass and short piano/organ stabs; hook/arp alternatives | Analog bass; Wheel/Phase/Wavetable organ and keys; restrained bright lead |
| G-FUNK | 88–102 | Relaxed funk backbeat, melodic bass answers, extended EP chords and a high legato lead with glide | Analog/SID bass and lead; Wheel/Phase keys; occasional Wavetable colour |
| NOCTURNAL HH | 132–156, half-time feel | Night-time hip-hop: one main snare on beat 3, deep held bass, slow dark harmony and a sparse upper phrase | Analog/Wavetable sub; Phase/Wheel/Wavetable pad; sparse bell or sine-key hook |
| TRIP HOP | 76–96 | Slow broken backbeat, quiet ghosts, late groove, minor/modal harmony and a recurring restrained motif | Analog/Lo-Fi bass; Phase/Wheel/Lo-Fi keys; dark upper motif |
| LOFI HOUSE | 112–124 | Four-on-the-floor foundation with softer timing, deep offbeat bass and warm recurring dub/keys chords | Analog bass; Lo-Fi/Phase/Wheel keys, with Wavetable also eligible |
| ABSTRACT IDM | 94–138 | Rule-based Euclidean drums, three-step velocity accents, sparse quartal harmony, cycle-3/5 arp and a 15-step melodic cycle at higher complexity | Phase/Lo-Fi/SID/Analog, plus limited Wavetable colour; no permanently running generative DSP |
| AMBIENT IDM | 104–128 | Gentle broken/electro drums beneath slow suspended/extended harmony and a repeating bell/sine-key figure; quieter high percussion | Phase/Wavetable/Wheel pads; Analog sub; Phase/Lo-Fi bell or sine-key motif |
| ATMOS BREAKS | 128–146 | Rolling syncopated breaks, quiet ghosts, deep sustained bass, short dub chords and an airy melodic figure | Analog/Wavetable sub; Phase/Wheel/Wavetable dub keys; soft bell/pluck |
| DRUNKEN HH | 76–96 | Anchored downbeat, pushed secondary kicks, delayed main snares and alternating early/late hat events; warm close-voiced chords and a short answering motif | Analog/Lo-Fi bass; Phase/Wheel/Lo-Fi EP; short keys or restrained lead |

The selected genre is also shown at full width below the four generated-track previews, so the knob card can remain compact. NOCTURNAL HH is the compact FM-1 display name for Nocturnal Hip-Hop. LOFI HOUSE and TRIP HOP omit hyphens for the small pixel display. These tempo ranges and weights are Rhythmer design choices, not historical definitions of a genre.

The three melodic tracks always retain bass and harmony. The remaining role is a weighted hook/arp choice driven by their GEN amounts; explicitly zeroed roles stay off. G-Funk therefore normally makes room for its lead instead of spending the only remaining track on an incidental arp. Each genre has three bass archetypes and three progression choices, multiple scales and seeded phrase development. Existing chord voice leading, motif development, cadence and kick/bass interaction remain active. Humanisation uses bounded role-specific cells; it does not add uncorrelated random offsets to every hit. Drum anchors, ghost dynamics and phrase-end answers remain distinct.

## The three later references

- [Aphex Twin, original Future Music interview reprinted by MusicRadar](https://www.musicradar.com/news/aphex-twin-interview-selected-ambient-works): the early records combine fluid electronic mood with varied construction. AMBIENT IDM is our melodic, rhythmic interpretation, with long harmonic holds, suspended/Lydian colour and a recurring upper figure. It does not try to turn every beat into a dense glitch fill, or reproduce a specific SAW melody.
- [Skee Mask, official Ilian Tape profile](https://iliantape.de/skee-mask/) describes rolling rhythm and electronic soundscapes, with techno/ambient influences. [The label's Compro release](https://iliantape.bandcamp.com/album/itlp04-compro) provides the reference context. ATMOS BREAKS balances rolling breaks and ghosts with sustained low end, dub-chord articulation and restrained spatial sends. Genre tempos and voice choices remain our design decisions.
- [Questlove's 2005 RBMA lecture](https://www.redbullmusicacademy.com/lectures/uestlove-phrenology-of-hip-hop/) discusses Dilla's dynamics; [his 2013 lecture](https://www.redbullmusicacademy.com/lectures/questlove-new-york-2013/) discusses deliberately unquantised programming. DRUNKEN HH therefore uses repeatable sound-dependent timing cells and velocity contrasts instead of one uniform swing percentage or independent random jitter. Default global swing is zero. HUMAN scales the displacement, and HUMAN zero restores grid timing. The existing firmware stores microtiming per step: simultaneous drum lanes intentionally move together, while separate kick, snare and hat events receive different timing. No new per-lane runtime queue or storage ABI is claimed.

## Synth selection and CPU policy

For each role, all six light GEN engines remain eligible: Analog, Phase, Lo-Fi, Wheel, SID-6581 and Wavetable. Genre/role-specific integer weights favour suitable engines rather than fixing one machine to a track. Bass presets get higher probability on bass; keys/pads on harmony; plucks and leads on the final role. Matching recipe keywords and avoiding repeated presets provide a second layer of selection. Non-bass roles exclude bass-tagged presets; Lo-Fi byte/noise modes remain out of automatic pitched roles. Manual engine selection is unrestricted.

GEN still changes only tracks 1–4. Tracks 5–6 are not repopulated with Acid or Break notes. At most one Wavetable is allocated automatically, counting Wavetable already assigned to untouched tracks. There is no new audio buffer, ordinary voice reduction, DSP synthesis engine or audio-interrupt random generator. The additional planning runs when GEN is invoked. Polyphony and existing dual-core scheduling remain as before; actual synth choice, notes and effects still affect render cost.

## Primary research and existing assets

- [Kaytranada, Red Bull Music Academy lecture](https://www.redbullmusicacademy.com/lectures/kaytranada-lecture/): the artist discusses swing, soul, hip-hop and house. Adaptation: an anchored club pulse with late offbeats and syncopated bass, rather than applying identical swing to every part.
- [Noah 40 Shebib, original GQ interview](https://www.gq.com/story/noah-shebib-40-drake-take-care-producer) and [Rolling Stone original profile](https://au.rollingstone.com/music/music-features/noah-40-shebib-profile-12476/): space and restrained production. Adaptation: slower harmonic movement, half-time backbeat, reduced upper density and a darker chord filter. This is a broad nocturnal hip-hop profile, not a Drake song replica.
- [Portishead, KEXP original interview](https://www.kexp.org/read/2019/8/22/portisheads-geoff-barrow-and-adrian-utley-breakdown-dummy-25-years-later/): sampling and arrangement are discussed by the original musicians. Adaptation: weight, recurring motifs and harmonic atmosphere; no recording or melody is copied.
- [Mall Grab, original Mixmag interview](https://mixmag.net/feature/hot-property-mall-grabs-had-a-dizzying-rise-to-becoming-a-house-music-star): rougher house context and the artist's approach. Adaptation: a clean pulse, softened keys and looser articulation. Deliberate heavy distortion is not required for the profile.
- [Autechre, Sound On Sound original interview](https://www.soundonsound.com/people/autechre): rule-based manipulation rather than indiscriminate random beats. Adaptation: constrained Euclidean pulses, cycle accents, bounded ratchets and an optional 15-step arp against the drum phrase. No Max/MSP system or model is ported.
- [GrooVAE, Magenta research](https://magenta.tensorflow.org/groovae): separates a drum score from expressive timing and dynamics. Design reference only; no neural model, training data or inference runtime is shipped.
- JIANT's existing 40 drum maps, 61 bass archetypes and seed streams remain the generation backbone. JR PLAY's role-aware timing concepts and JR TREK's selected chord assets already imported into Rhythmer are reused. The selected SHLD chord data is MIT-licensed, pinned and included with attribution; see NEXT-0.1.1.md. No new external music recordings are included by this genre update.

## Acceptance and limits

Native checks cover 18 refined profiles × 12 seeds, seeded variation, chord presence, bounded note counts, role engine variety, safe engine selection, one-Wavetable ceiling, half-time / four-kick / glide / 15-step identities and the complete menu permutation. Actual-FW emulator checks cover one arrangement from each profile plus Acid, MD and six-track ensembles. These objective checks establish that the intended rules execute; they do not prove every seed sounds good. Listening on the real FM-1 and long dense/FX-heavy sessions remain required for final musical and performance judgement. No popularity ranking is asserted.

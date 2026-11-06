# Break track filter, envelope and LFO

The Break machine now routes its actual audio through the ordinary track filter and a control ADSR before the existing track/mix FX. EDIT FILTER controls LP, HP, BP and COMB, cutoff and resonance. The Amenity SOUND FILTER remains a separate performance tone control.

ENV attack, decay, sustain, release and LOOP shape the audio. ENV FILTER sweeps the ordinary filter. Sequence NOTE events and live chops/rolls trigger the control envelope. TIE holds it. Source A/B and RETURN are modifiers and do not retrigger it. Releasing a live key while a sequence is held retains the underlying envelope gate. A longer sequence release keeps the source playing until that release completes; the default zero-release path retains immediate sequence-off behavior.

LFO 1's direct FILTER and AMP destinations work on Break. Both LFOs can drive CUT or AMP using the four modulation slots. ENV can be used as a modulation-slot source, including the Amenity FILTER destination; the stored knob value is restored after rendering. LFO/ENV pitch modulation sets Amenity's existing bounded transpose, up to one octave either way, for following source hits. These controls share the same parameter IDs and project format as the other tracks.

Break's unused voice slot 0 stores control state while `active` remains zero. It does not allocate or consume a shared synthesis voice. Sample memory and the shared twelve-/load-relief-ten-voice budget are unchanged. Filter transition scratch is now separate for each core, adding 128 bytes of static RAM and avoiding a shared-scratch collision when two tracks change filter type simultaneously.

Fresh firmware settings and NEW PROJECT start Break on A / Amen with a 32-step straight sequence: slices 0–15, each followed by a TIE, 1/16 division, full gate, two bars. Assigning Break to an empty track also supplies this starting pattern. Loading saved projects and selecting a sample/preset on an existing Break track do not overwrite edited patterns or a saved B choice. Browser workspaces for a previous firmware hash remain their own saves; create a new project if you want factory defaults.

Validation uses actual PCM from the complete host rendering path, source-only/live-key/sequence gate cases, saved B round-trips, fresh/default initialization, native firmware playback, multicore timing and the unchanged integrated-bank loader. Physical playback of the new candidate remains a separate owner test.

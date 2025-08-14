use ucra::*;

#[test]
fn basic_render_flow() {
    let mut eng = Engine::new().unwrap();
    let info = eng.get_info().unwrap();
    assert!(info.contains("UCRA"));

    let note = NoteSegment { start_sec: 0.0, duration_sec: 0.05, midi_note: 60, velocity: 127, lyric: None, f0_override: None, env_override: None };
    let notes = [note];
    let cfg = RenderConfig::new(44100, 1, &notes);
    let out = eng.render(&cfg).unwrap();
    assert!(out.frames() > 0);
    let pcm = out.pcm().unwrap();
    assert_eq!(pcm.len(), out.frames() as usize * out.channels() as usize);
}

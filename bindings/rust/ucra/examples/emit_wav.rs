use std::fs::File;
use std::io::Write;

fn write_wav_float32(path: &str, pcm: &[f32], sample_rate: u32, channels: u32) -> std::io::Result<()> {
    let mut f = File::create(path)?;
    let data_size = (pcm.len() * 4) as u32;
    let file_size = data_size + 36;
    f.write_all(b"RIFF")?;
    f.write_all(&file_size.to_le_bytes())?;
    f.write_all(b"WAVE")?;
    f.write_all(b"fmt ")?;
    f.write_all(&16u32.to_le_bytes())?;
    f.write_all(&3u16.to_le_bytes())?; // IEEE float
    f.write_all(&(channels as u16).to_le_bytes())?;
    f.write_all(&sample_rate.to_le_bytes())?;
    let byte_rate = sample_rate * channels * 4;
    f.write_all(&byte_rate.to_le_bytes())?;
    let block_align: u16 = (channels * 4) as u16;
    f.write_all(&block_align.to_le_bytes())?;
    f.write_all(&32u16.to_le_bytes())?;
    f.write_all(b"data")?;
    f.write_all(&data_size.to_le_bytes())?;
    let bytes: &[u8] = unsafe { std::slice::from_raw_parts(pcm.as_ptr() as *const u8, pcm.len() * 4) };
    f.write_all(bytes)?;
    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut eng = ucra::Engine::new()?;
    let note = ucra::NoteSegment { start_sec: 0.0, duration_sec: 2.0, midi_note: 67, velocity: 120, lyric: None, f0_override: None, env_override: None };
    let notes = [note];
    let cfg = ucra::RenderConfig::new(44100, 1, &notes);
    let out = eng.render(&cfg)?;
    let pcm = out.pcm().ok_or("no pcm")?;
    write_wav_float32("rust_sample_output.wav", pcm, out.sample_rate(), out.channels())?;
    println!("Wrote rust_sample_output.wav");
    Ok(())
}

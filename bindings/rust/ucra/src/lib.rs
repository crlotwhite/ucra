use std::{marker::PhantomData, mem::MaybeUninit, ptr};

use thiserror::Error;
use ucra_sys as sys;

#[derive(Debug, Error)]
pub enum Error {
    #[error("invalid argument")]
    InvalidArgument,
    #[error("out of memory")]
    OutOfMemory,
    #[error("not supported")]
    NotSupported,
    #[error("internal error")]
    Internal,
    #[error("file not found")]
    FileNotFound,
    #[error("invalid json")]
    InvalidJson,
    #[error("invalid manifest")]
    InvalidManifest,
    #[error("unknown error code: {0}")]
    Unknown(i32),
}

fn check(code: sys::UCRA_Result) -> Result<()> {
    match code as u32 {
        0 => Ok(()),
        1 => Err(Error::InvalidArgument),
        2 => Err(Error::OutOfMemory),
        3 => Err(Error::NotSupported),
        4 => Err(Error::Internal),
        5 => Err(Error::FileNotFound),
        6 => Err(Error::InvalidJson),
        7 => Err(Error::InvalidManifest),
        other => Err(Error::Unknown(other as i32)),
    }
}

pub type Result<T> = std::result::Result<T, Error>;

pub struct Engine {
    raw: sys::UCRA_Handle,
}

impl Engine {
    pub fn new() -> Result<Self> {
        let mut handle: sys::UCRA_Handle = ptr::null_mut();
    unsafe { check(sys::ucra_engine_create(&mut handle as *mut _, ptr::null(), 0))? };
        Ok(Self { raw: handle })
    }

    pub fn get_info(&self) -> Result<String> {
        let mut buf = vec![0u8; 128];
        let res = unsafe {
            sys::ucra_engine_getinfo(self.raw, buf.as_mut_ptr() as *mut i8, buf.len())
        };
    check(res)?;
        let nul = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
        let s = String::from_utf8_lossy(&buf[..nul]).to_string();
        Ok(s)
    }

    pub fn render(&mut self, config: &RenderConfig<'_>) -> Result<RenderResult<'_>> {
        let mut out = MaybeUninit::<sys::UCRA_RenderResult>::zeroed();
    unsafe { check(sys::ucra_render(self.raw, &config.raw, out.as_mut_ptr()))? };
        let out = unsafe { out.assume_init() };
        Ok(RenderResult { raw: out, _marker: PhantomData })
    }
}

impl Drop for Engine {
    fn drop(&mut self) {
        unsafe { sys::ucra_engine_destroy(self.raw) }
    }
}

#[derive(Default)]
pub struct NoteSegment<'a> {
    pub start_sec: f64,
    pub duration_sec: f64,
    pub midi_note: i16,
    pub velocity: u8,
    pub lyric: Option<&'a str>,
    pub f0_override: Option<&'a F0Curve<'a>>,
    pub env_override: Option<&'a EnvCurve<'a>>,
}

impl<'a> From<&NoteSegment<'a>> for sys::UCRA_NoteSegment {
    fn from(n: &NoteSegment<'a>) -> Self {
        // Avoid temporary CString leaks for now; pass null lyric.
        let c_lyric = ptr::null();
        sys::UCRA_NoteSegment {
            start_sec: n.start_sec,
            duration_sec: n.duration_sec,
            midi_note: n.midi_note,
            velocity: n.velocity,
            lyric: c_lyric,
            f0_override: n.f0_override.map(|c| &c.raw as *const _).unwrap_or(ptr::null()),
            env_override: n.env_override.map(|c| &c.raw as *const _).unwrap_or(ptr::null()),
        }
    }
}

pub struct F0Curve<'a> { raw: sys::UCRA_F0Curve, _p: PhantomData<&'a ()> }
impl<'a> F0Curve<'a> { pub fn new(time_sec: &'a [f32], f0_hz: &'a [f32]) -> Self { assert_eq!(time_sec.len(), f0_hz.len()); Self { raw: sys::UCRA_F0Curve { time_sec: time_sec.as_ptr(), f0_hz: f0_hz.as_ptr(), length: time_sec.len() as u32 }, _p: PhantomData } } }

pub struct EnvCurve<'a> { raw: sys::UCRA_EnvCurve, _p: PhantomData<&'a ()> }
impl<'a> EnvCurve<'a> { pub fn new(time_sec: &'a [f32], value: &'a [f32]) -> Self { assert_eq!(time_sec.len(), value.len()); Self { raw: sys::UCRA_EnvCurve { time_sec: time_sec.as_ptr(), value: value.as_ptr(), length: time_sec.len() as u32 }, _p: PhantomData } } }

pub struct RenderConfig<'a> {
    raw: sys::UCRA_RenderConfig,
    // Own the converted C notes so pointers in raw remain valid
    c_notes: Vec<sys::UCRA_NoteSegment>,
    _p: PhantomData<&'a ()>,
}

impl<'a> RenderConfig<'a> {
    pub fn new(sample_rate: u32, channels: u32, notes: &'a [NoteSegment<'a>]) -> Self {
        // Convert notes into owned C array to ensure lifetime safety across FFI call
        let c_notes: Vec<sys::UCRA_NoteSegment> = notes.iter().map(|n| sys::UCRA_NoteSegment::from(n)).collect();
        let raw = sys::UCRA_RenderConfig {
            sample_rate,
            channels,
            block_size: 0,
            flags: 0,
            notes: c_notes.as_ptr(),
            note_count: c_notes.len() as u32,
            options: ptr::null(),
            option_count: 0,
        };
        Self { raw, c_notes, _p: PhantomData }
    }
}

pub struct RenderResult<'a> {
    raw: sys::UCRA_RenderResult,
    _marker: PhantomData<&'a ()>,
}

impl<'a> RenderResult<'a> {
    pub fn pcm(&self) -> Option<&[f32]> {
        if self.raw.pcm.is_null() || self.raw.frames == 0 { return None; }
        let len = (self.raw.frames as usize) * (self.raw.channels as usize);
        unsafe { Some(std::slice::from_raw_parts(self.raw.pcm, len)) }
    }
    pub fn frames(&self) -> u64 { self.raw.frames }
    pub fn channels(&self) -> u32 { self.raw.channels }
    pub fn sample_rate(&self) -> u32 { self.raw.sample_rate }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn engine_info() {
        let eng = Engine::new().unwrap();
        let info = eng.get_info().unwrap();
        assert!(info.contains("UCRA Reference Engine"));
    }

    #[test]
    fn render_sine() {
        let mut eng = Engine::new().unwrap();
    let note = NoteSegment { start_sec: 0.0, duration_sec: 0.1, midi_note: 69, velocity: 100, lyric: None, f0_override: None, env_override: None };
    let notes = [note];
    let cfg = RenderConfig::new(44100, 1, &notes);
        let out = eng.render(&cfg).unwrap();
        assert!(out.frames() > 0);
        let pcm = out.pcm().unwrap();
        assert!(!pcm.is_empty());
    }
}

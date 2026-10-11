"""Resolve local tools without hard-coded user paths or bundled fonts."""
from pathlib import Path
import os, shutil

def ffmpeg():
    candidate=os.environ.get('FFMPEG_BINARY') or shutil.which('ffmpeg')
    if candidate:return candidate
    fallback=Path('C:/ffmpeg/bin/ffmpeg.exe')
    if fallback.is_file():return str(fallback)
    raise RuntimeError('Install FFmpeg on PATH or set FFMPEG_BINARY to its executable.')

def font_path(bold=False):
    override=os.environ.get('ANIMATION_FONT_BOLD' if bold else 'ANIMATION_FONT')
    candidates=[override] if override else []
    candidates += [
        'C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf',
        '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
        '/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf',
        '/System/Library/Fonts/Supplemental/Arial Bold.ttf' if bold else '/System/Library/Fonts/Supplemental/Arial.ttf',
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():return str(candidate)
    raise RuntimeError('Set ANIMATION_FONT (and optionally ANIMATION_FONT_BOLD) to a Unicode TrueType font.')

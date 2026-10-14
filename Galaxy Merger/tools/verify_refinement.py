"""Verify a V5/V6/V7 particle recording against its massless-tracer refinement.

Checks every physical sample byte, original mass/name, and zero tracer masses.
Reports intrinsic broadband light changes (restyling can intentionally change them).
python tools/verify_refinement.py source.traj refined.traj --output verification.json
"""
import argparse
import json
import mmap
from pathlib import Path
import struct


def header(stream):
    magic = stream.read(8)
    if magic not in (b"PHYSTRJ5", b"PHYSTRJ6", b"PHYSTRJ7"):
        raise ValueError("verification requires V5, V6 or V7")
    version = int(chr(magic[-1]))
    n, samples = struct.unpack("<II", stream.read(8))
    if version >= 6:
        stream.read(24)
    bodies = []
    for _ in range(n):
        length = struct.unpack("<I", stream.read(4))[0]
        name = stream.read(length).decode("utf-8")
        count = 14 if version >= 7 else 12 if version >= 6 else 11
        fields = struct.unpack(f"<{count}d", stream.read(8 * count))
        weight = fields[11] if version >= 6 and fields[11] >= 0 else fields[10]
        luminance = sum(a*b for a, b in zip(fields[1:4], (.2126, .7152, .0722)))
        bodies.append((name, fields[10], weight * fields[5] * luminance))
    cohorts = b""
    if version >= 7:
        count = struct.unpack("<I", stream.read(4))[0]
        cohorts = stream.read(count * 20)
        assert len(cohorts) == count * 20, "truncated cohorts"
    return n, samples, stream.tell(), bodies, version, cohorts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("refined", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.resolve() in (args.source.resolve(), args.refined.resolve()):
        parser.error("verification output must differ from both recordings")
    with args.source.open("rb") as a, args.refined.open("rb") as b:
        na, sa, oa, ba, va, ca = header(a)
        nb, sb, ob, bb, vb, cb = header(b)
        assert ca == cb, "stellar formation history changed"
        wa, wb = (81 if va >= 7 else 73), (81 if vb >= 7 else 73)
        assert nb > na and sa == sb, "expected additional tracers and unchanged snapshot count"
        assert [r[:2] for r in ba] == [r[:2] for r in bb[:na]], "physical body identity/mass changed"
        assert all(row[1] == 0 for row in bb[na:]), "tracers must have zero mass"
        with mmap.mmap(a.fileno(), 0, access=mmap.ACCESS_READ) as ma, mmap.mmap(b.fileno(), 0, access=mmap.ACCESS_READ) as mb:
            for frame in range(sa):
                assert ma[oa:oa+80] == mb[ob:ob+80], f"time/diagnostics changed at {frame}"
                for start, width in [(0, 24), (24, 24), (48, 1), (49, 8), (57, 8), (65, 8)]:
                    x, y = oa + 80 + start*na, ob + 80 + start*nb
                    assert ma[x:x+width*na] == mb[y:y+width*na], f"physical state changed at {frame}"
                if va >= 7 and vb >= 7:
                    x, y = oa+80+73*na, ob+80+73*nb
                    assert ma[x:x+8*na] == mb[y:y+8*na], f"gas fractions changed at {frame}"
                x, y = oa+80+wa*na, ob+80+wb*nb
                assert struct.unpack_from("<I", ma, x)[0] == 0, "particle-only recordings required"
                assert ma[x:x+20] == mb[y:y+20], f"field metadata changed at {frame}"
                oa += 100+wa*na
                ob += 100+wb*nb
            assert oa == len(ma) and ob == len(mb), "unexpected trailing or truncated data"
    before, after = sum(r[2] for r in ba), sum(r[2] for r in bb)
    report = dict(source=str(args.source.resolve()), refined=str(args.refined.resolve()),
                  physical_bodies=na, massless_tracers=nb-na, snapshots=sa,
                  physical_state_exactly_preserved=True, intrinsic_luminance_before=before,
                  intrinsic_luminance_after=after, relative_luminance_change=(after-before)/max(abs(before), 1e-30))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

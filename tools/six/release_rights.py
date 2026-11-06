"""Rights readiness is independent of attribution or the software license."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def public_audio_release_blockers(root=ROOT):
    record=json.loads((root/'site/audio-rights.json').read_text())
    # These original recording assets have no documented redistribution permission.
    # Do not infer permission from the GPL, a notice, a public download or credit.
    return [f"{x['title']}: no documented permission to redistribute this recording in firmware/sample bundles"
            for x in record['samples'] if x['status']!='cleared-for-firmware-redistribution' or not x.get('firmwareRedistributionPermission') or not x.get('licenseDocument') or not (root/x['licenseDocument']).is_file()]
if __name__=='__main__':
    blockers=public_audio_release_blockers()
    print('\n'.join(['PUBLIC AUDIO RELEASE BLOCKED',*blockers]) if blockers else 'Audio documentation ready for review; this is not legal clearance advice.')
    raise SystemExit(2 if blockers else 0)

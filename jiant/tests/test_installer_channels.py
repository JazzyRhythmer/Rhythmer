import importlib.util
from pathlib import Path
import unittest

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('make_site', root/'web/make_site.py')
site = importlib.util.module_from_spec(spec)
spec.loader.exec_module(site)

class InstallerChannels(unittest.TestCase):
    def test_public_stable_first_then_classic_order(self):
        # (2026-10-30) the stable one (web/STABLE) stays the default; the others follow in web/CLASSIC's order
        entries = [{'version':'V10-4b5f5c6','edition':'','status':'stable'},
                   {'version':'v0.9.9-par11-3d5566f','edition':'','status':'public'}]
        result = site.installable_versions(entries, False)
        self.assertEqual([e['version'] for e in result], ['V10-4b5f5c6','v0.9.9-par11-3d5566f'])

    def test_v10_alone(self):
        # (2026-10-30) JIANT FM V10 is the new base: web/CLASSIC offers nothing else
        result = site.installable_versions([{'version':'V10-4b5f5c6','edition':'','status':'stable'}], False)
        self.assertEqual([e['version'] for e in result], ['V10-4b5f5c6'])

    def test_all_par14_variants_blocked_even_if_mislabeled(self):
        add = {'version':'v0.9.9-par10d-beta2-42947e9-preview','edition':'ADDITIVE + CONTROLS','status':'beta'}
        for suffix in ['beta1-86b85ad-preview','beta2-4be8592-preview','beta4-4d3156d-preview','future-1234567']:
            bad = {'version':'v0.9.9-par14-'+suffix,'edition':'SAMPLE','status':'public'}
            self.assertEqual(site.installable_versions([bad,add], True), [add])

    def test_withdrawn_only_fails_closed(self):
        with self.assertRaises(SystemExit):
            site.installable_versions([{'version':'v0.9.9-par14-beta4','edition':'SAMPLE'}],True)

    def test_beta_still_forbidden_in_public(self):
        with self.assertRaises(SystemExit):
            site.installable_versions([{'version':'v0.9.9-par10d-beta2','edition':'ADDITIVE','status':'beta'}],False)

if __name__ == '__main__':
    unittest.main()

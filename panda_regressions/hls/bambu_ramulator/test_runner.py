# SPDX-License-Identifier: Apache-2.0
"""Guard against false-green results in the integration runner."""
from pathlib import Path
import tempfile
import unittest

from run_tests import parallelism, validate


class EvidenceChecks(unittest.TestCase):
    def test_success_without_real_ramulator_activity_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'bambu_results.xml').write_text(
                '<application><timing><simulation return_value="0"/></timing>'
                '<evaluation CYCLES="100"/></application>')
            (root / 'run.log').write_text('Ramulator2 successfully initialized\n')
            with self.assertRaisesRegex(ValueError, 'actual Ramulator read completions'):
                validate(root, 0, False, 5000)

    def test_wrong_functional_result_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'bambu_results.xml').write_text(
                '<application><timing><simulation return_value="1"/></timing>'
                '<evaluation CYCLES="100"/></application>')
            with self.assertRaisesRegex(ValueError, 'RTL/golden'):
                validate(root, 0, False, 5000)

    def test_two_banks_used_serially_are_not_parallel_traffic(self):
        header = '''$scope module bench $end
$scope module system $end
$var wire 1 C clock $end
$scope module DUT $end
$var wire 4 B Mout_oe_ram [3:0] $end
$scope module top $end
$var wire 1 A0 sig_in_Mout_oe_ram_0 $end
$var wire 1 A1 sig_in_Mout_oe_ram_1 $end
$var wire 1 A2 sig_in_Mout_oe_ram_2 $end
$var wire 1 A3 sig_in_Mout_oe_ram_3 $end
$upscope $end
$upscope $end
$upscope $end
$upscope $end
$enddefinitions $end
#0
0C
0A0
0A1
0A2
0A3
b0000 B
'''
        serial = '#5\n1A0\nb0001 B\n1C\n#10\n0C\n0A0\n#15\n1A1\nb0010 B\n1C\n#20\n0C\n'
        concurrent = '#5\n1A0\n1A1\nb0011 B\n1C\n#10\n0C\n'
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'trace.vcd'
            path.write_text(header + serial)
            with self.assertRaisesRegex(ValueError, 'insufficient parallel'):
                parallelism(path)
            path.write_text(header + concurrent)
            evidence = parallelism(path)
            self.assertEqual(evidence['peak_parallel_requests'], 2)
            self.assertEqual(evidence['active_banks'], [0, 1])


if __name__ == '__main__':
    unittest.main()

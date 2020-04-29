# This file is part of obs_subaru.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (http://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Tests of the butler cli subcommands with HSC."""

import unittest

import lsst.utils.tests
from lsst.obs.base.cli.butler_cmd_test import ButlerCmdTestBase


class TestButlerCmd(ButlerCmdTestBase, lsst.utils.tests.TestCase):
    def setUp(self):
        self.instrument_class = "lsst.obs.subaru.HyperSuprimeCam"
        self.instrument_name = "HSC"


if __name__ == '__main__':
    unittest.main()

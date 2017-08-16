#
# LSST Data Management System
# Copyright 2016 LSST Corporation.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
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
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.
#
import os
import sys
import unittest

import lsst.utils.tests
from lsst.utils import getPackageDir
from lsst.afw.geom import Extent2I
import lsst.obs.base.tests
import lsst.obs.test


class TestObsTest(lsst.obs.base.tests.ObsTests, lsst.utils.tests.TestCase):
    def setUp(self):
        product_dir = getPackageDir('obs_test')
        data_dir = os.path.join(product_dir, 'data', 'input')

        butler = lsst.daf.persistence.Butler(root=data_dir)
        mapper = lsst.obs.test.TestMapper(root=data_dir)
        dataIds = {'raw': {'visit': 1, 'filter': 'g'},
                   'bias': {'visit': 1},
                   'flat': {'visit': 1},
                   'dark': unittest.SkipTest
                   }
        self.setUp_tests(butler, mapper, dataIds)

        ccdExposureId_bits = 41
        exposureIds = {'raw': 1, 'bias': 1, 'flat': 1}
        filters = {'raw': 'g', 'bias': '_unknown_', 'flat': 'g'}
        exptimes = {'raw': 15.0, 'bias': 0.0, 'flat': 0.0}
        detectorIds = {'raw': 0, 'bias': 0, 'flat': 0}
        detector_names = {'raw': '0', 'bias': '0', 'flat': '0'}
        detector_serials = {'raw': '0000011', 'bias': '0000011', 'flat': '0000011'}
        dimensions = {'raw': Extent2I(1026, 2000),
                      'bias': Extent2I(1018, 2000),
                      'flat': Extent2I(1018, 2000)
                      }
        sky_origin = (79.24522, -9.702295)
        raw_subsets = (({'level': 'sensor', 'filter': 'g'}, 2),
                       ({'level': 'sensor', 'visit': 1}, 1),
                       ({'level': 'filter', 'visit': 1}, 1),
                       ({'level': 'visit', 'filter': 'g'}, 2)
                       )
        linearizer_type = unittest.SkipTest
        self.setUp_butler_get(ccdExposureId_bits=ccdExposureId_bits,
                              exposureIds=exposureIds,
                              filters=filters,
                              exptimes=exptimes,
                              detectorIds=detectorIds,
                              detector_names=detector_names,
                              detector_serials=detector_serials,
                              dimensions=dimensions,
                              sky_origin=sky_origin,
                              raw_subsets=raw_subsets,
                              linearizer_type=linearizer_type
                              )

        path_to_raw = os.path.join(data_dir, "raw", "raw_v1_fg.fits.gz")
        keys = set(('filter', 'patch', 'skyTile', 'tract', 'visit', 'pixel_id'))
        query_format = ["visit", "filter"]
        queryMetadata = (({'visit': 1}, [(1, 'g')]),
                         ({'visit': 2}, [(2, 'g')]),
                         ({'visit': 3}, [(3, 'r')]),
                         ({'filter': 'g'}, [(1, 'g'), (2, 'g')]),
                         ({'filter': 'r'}, [(3, 'r')]),
                         )
        map_python_type = 'lsst.afw.image.DecoratedImageU'
        map_cpp_type = 'DecoratedImageU'
        map_storage_name = 'FitsStorage'
        metadata_output_path = os.path.join('processCcd_metadata', 'v1_fg.boost')
        raw_filename = 'raw_v1_fg.fits.gz'
        default_level = 'visit'
        raw_levels = (('skyTile', set(['filter'])),
                      ('filter', set(['filter', 'visit'])),
                      ('visit', set(['filter', 'visit']))
                      )
        self.setUp_mapper(output=data_dir,
                          path_to_raw=path_to_raw,
                          keys=keys,
                          query_format=query_format,
                          queryMetadata=queryMetadata,
                          metadata_output_path=metadata_output_path,
                          map_python_type=map_python_type,
                          map_cpp_type=map_cpp_type,
                          map_storage_name=map_storage_name,
                          raw_filename=raw_filename,
                          default_level=default_level,
                          raw_levels=raw_levels,
                          )

        self.setUp_camera(camera_name='test',
                          n_detectors=1,
                          first_detector_name='0',
                          )

        super(TestObsTest, self).setUp()


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == '__main__':
    setup_module(sys.modules[__name__])
    unittest.main()

# Copyright (C) 2017  HSC Software Team
# Copyright (C) 2017  Satoshi Kawanomoto
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

"""Module to calculate instrument rotator angle at start and end of observation"""

__all__ = ["inrStartEnd"]

import numpy as np
from astropy.io import fits
import lsst.geom

# fixed parameters
ltt_d = 19.82556  # dome latitude in degree
lng_d = -155.47611  # dome longitude in degree
mjd_J2000 = 51544.5  # mjd at J2000.0 (2000/01/01.5)

# refraction index of air
# T=273.15[K], P=600[hPa], Pw=1.5[hPa], lambda=0.55[um]
air_idx = 1.0 + 1.7347e-04

# scale height of air
air_sh = 0.00130


def _mjd2jc2000(mjd):
    """convert mjd to Julian century (J2000.0 origin)"""
    jc2000 = (mjd - mjd_J2000) / 36525.0
    return jc2000


def _precessionMatrix(jc2000):
    """create precession matrix at the given time in Julian century"""
    zeta_A = np.deg2rad((2306.2181*jc2000 + 0.30188*jc2000**2.0 + 0.017998*jc2000**3.0)/3600.0)
    z_A = np.deg2rad((2306.2181*jc2000 + 1.09468*jc2000**2.0 + 0.018203*jc2000**3.0)/3600.0)
    theta_A = np.deg2rad((2004.3109*jc2000 - 0.42665*jc2000**2.0 - 0.041833*jc2000**3.0)/3600.0)
    precMat = np.matrix([[+np.cos(zeta_A)*np.cos(theta_A)*np.cos(z_A) - np.sin(zeta_A)*np.sin(z_A),
                          -np.sin(zeta_A)*np.cos(theta_A)*np.cos(z_A) - np.cos(zeta_A)*np.sin(z_A),
                          -np.sin(theta_A)*np.cos(z_A)],
                         [+np.cos(zeta_A)*np.cos(theta_A)*np.sin(z_A) + np.sin(zeta_A)*np.cos(z_A),
                          -np.sin(zeta_A)*np.cos(theta_A)*np.sin(z_A) + np.cos(zeta_A)*np.cos(z_A),
                          -np.sin(theta_A)*np.sin(z_A)],
                         [+np.cos(zeta_A)*np.sin(theta_A),
                          -np.sin(zeta_A)*np.sin(theta_A),
                          +np.cos(theta_A)]])
    return precMat


def _mjd2gmst(mjd):
    """convert mjd to GMST(Greenwich mean sidereal time)"""
    mjd_f = mjd % 1
    jc2000 = _mjd2jc2000(mjd)
    gmst_s = ((6.0*3600.0 + 41.0*60.0 + 50.54841) +
              8640184.812866*jc2000 + 0.093104*jc2000**2.0 - 0.0000062*jc2000**3.0 +
              mjd_f*86400.0)
    gmst_d = (gmst_s % 86400)/240.0
    return gmst_d


def _gmst2lmst(gmst_d):
    """convert GMST to LMST(mean local sidereal time)"""
    lmst_d = (gmst_d + lng_d) % 360
    return lmst_d


def _sph2vec(ra_d, de_d):
    """convert spherical coordinate to the Cartesian coordinates (vector)"""
    ra_r = np.deg2rad(ra_d)
    de_r = np.deg2rad(de_d)
    vec = np.array([[np.cos(ra_r)*np.cos(de_r)],
                    [np.sin(ra_r)*np.cos(de_r)],
                    [np.sin(de_r)]])
    return vec


def _vec2sph(vec):
    """convert the Cartesian coordinates vector to shperical coordinates"""
    ra_r = np.arctan2(vec[1, 0], vec[0, 0])
    de_r = np.arcsin(vec[2, 0])
    ra_d = np.rad2deg(ra_r)
    de_d = np.rad2deg(de_r)
    return ra_d, de_d


def _ra2ha(ra_d, lst_d):
    """convert right ascension to hour angle at given LST"""
    ha_d = (lst_d - ra_d)%360
    return ha_d


def _eq2hz(ha_d, de_d):
    """convert equatorial coordinates to the horizontal coordinates"""
    ltt_r = np.deg2rad(ltt_d)
    ha_r = np.deg2rad(ha_d)
    de_r = np.deg2rad(de_d)
    zd_r = np.arccos(+np.sin(ltt_r)*np.sin(de_r) + np.cos(ltt_r)*np.cos(de_r)*np.cos(ha_r))
    az_r = np.arctan2(+np.cos(de_r)*np.sin(ha_r),
                      -np.cos(ltt_r)*np.sin(de_r) + np.sin(ltt_r)*np.cos(de_r)*np.cos(ha_r))
    zd_d = np.rad2deg(zd_r)
    az_d = np.rad2deg(az_r)
    al_d = 90.0 - zd_d
    return al_d, az_d


def _air_idx():
    """return the air refraction index"""
    return air_idx


def _atm_ref(al_d):
    """return the atmospheric refraction at given altitude"""
    if al_d > 20.0:
        zd_r = np.deg2rad(90.0 - al_d)
    else:
        zd_r = np.deg2rad(70.0)
    r0 = _air_idx()-1.0
    sh = air_sh
    R0 = (1.0 - sh)*r0 - sh*r0**2/2.0 + sh**2*r0*2.0
    R1 = r0**2/2.0 + r0**3/6.0 - sh*r0 - sh*r0**2*11.0/4.0 + sh**2*r0*5.0
    R2 = r0**3 - sh*r0**2*9.0/4.0 + sh**2*r0*3.0
    R = R0*np.tan(zd_r) + R1*(np.tan(zd_r))**3 + R2*(np.tan(zd_r))**5
    return np.rad2deg(R)


def _mal2aal(mal_d):
    """convert mean altitude to apparent altitude"""
    aal_d = mal_d + _atm_ref(mal_d)
    return aal_d


def _pos2adt(al_t_d, al_s_d, delta_az_d):
    """convert altitudes of telescope and star and relative azimuth to angular distance and position angle"""
    zd_t_r = np.deg2rad(90.0-al_t_d)
    zd_s_r = np.deg2rad(90.0-al_s_d)
    daz_r = np.deg2rad(delta_az_d)

    ad_r = np.arccos(np.cos(zd_t_r)*np.cos(zd_s_r) + np.sin(zd_t_r)*np.sin(zd_s_r)*np.cos(daz_r))

    if ad_r > 0.0:
        pa_r = np.arcsin(np.sin(zd_s_r)*np.sin(daz_r)/np.sin(ad_r))
    else:
        pa_r = 0.0
    ad_d = np.rad2deg(ad_r)
    pa_d = np.rad2deg(pa_r)
    if (zd_t_r < zd_s_r):
        pa_d = 180.0 - pa_d

    return ad_d, pa_d


def _addpad2xy(ang_dist_d, p_ang_d, inr_d):
    """convert angular distance, position angle, and instrument rotator angle to position on the cold plate"""
    t = 90.0-(p_ang_d-inr_d)
    x = np.cos(np.deg2rad(t))
    y = np.sin(np.deg2rad(t))
    return x, y


def _gsCPposNorth(ra_t_d, de_t_d, inr_d, mjd):
    jc2000 = _mjd2jc2000(mjd)
    pm = _precessionMatrix(jc2000)

    vt = _sph2vec(ra_t_d, de_t_d)
    vt_mean = np.dot(pm, vt)

    (mean_ra_t_d, mean_de_t_d) = _vec2sph(vt_mean)
    mean_ra_s_d = mean_ra_t_d
    mean_de_s_d = mean_de_t_d+0.75

    gmst_d = _mjd2gmst(mjd)
    lmst_d = _gmst2lmst(gmst_d)

    mean_ha_t_d = _ra2ha(mean_ra_t_d, lmst_d)
    mean_ha_s_d = _ra2ha(mean_ra_s_d, lmst_d)

    (mean_al_t_d, mean_az_t_d) = _eq2hz(mean_ha_t_d, mean_de_t_d)
    (mean_al_s_d, mean_az_s_d) = _eq2hz(mean_ha_s_d, mean_de_s_d)

    apparent_al_t_d = _mal2aal(mean_al_t_d)
    apparent_al_s_d = _mal2aal(mean_al_s_d)

    delta_az_d = mean_az_s_d - mean_az_t_d

    (ang_dist_d, p_ang_d) = _pos2adt(apparent_al_t_d, apparent_al_s_d, delta_az_d)

    (x, y) = _addpad2xy(ang_dist_d, p_ang_d, inr_d)
    return x, y


def _getDataArrayFromFITSFileWithHeader(pathToFITSFile):
    """return array of pixel data"""
    fitsfile = fits.open(pathToFITSFile)
    dataArray = fitsfile[0].data
    fitsHeader = fitsfile[0].header
    fitsfile.close()
    return dataArray, fitsHeader


def _minorArc(angle1, angle2):
    """e.g. input (-179, 179) -> output (-179, -181)"""

    angle1 = (angle1 + 180.0) % 360 - 180.0
    angle2 = (angle2 + 180.0) % 360 - 180.0

    if angle1 < angle2:
        if angle2 - angle1 > 180.0:
            angle2 -= 360.0
    elif angle2 < angle1:
        if angle1 - angle2 > 180.0:
            angle1 -= 360.0

    # Try to place [angle1, angle2] within [-270, +270]

    if min(angle1, angle2) < -270.0:
        angle1 += 360.0
        angle2 += 360.0
    if max(angle1, angle2) > 270.0:
        angle1 -= 360.0
        angle2 -= 360.0

    return angle1, angle2


def inrStartEnd(visitInfo):
    """Calculate instrument rotator angle for start and end of exposure

    Parameters
    ----------
    visitInfo : `lsst.afw.image.VisitInfo`
        Visit info for the exposure to calculate correction.

    Returns
    -------
    start : `float`
        Instrument rotator angle at start of exposure, degrees.
    end : `float`
        Instrument rotator angle at end of exposure, degrees.
    """

    inst_pa = 270.0 - visitInfo.getBoresightRotAngle().asAngularUnits(lsst.geom.degrees)
    ra_t_sp, de_t_sp = visitInfo.getBoresightRaDec()

    ra_t_d = ra_t_sp.asAngularUnits(lsst.geom.degrees)
    de_t_d = de_t_sp.asAngularUnits(lsst.geom.degrees)

    mjd_str = visitInfo.getDate().get() - 0.5*visitInfo.getExposureTime()/86400.0
    mjd_end = visitInfo.getDate().get() + 0.5*visitInfo.getExposureTime()/86400.0

    inr_d = 0.00

    (x, y) = _gsCPposNorth(ra_t_d, de_t_d, inr_d, mjd_str)
    x_inr_str = 90.0 - np.rad2deg(np.arctan2(y, x)) + inst_pa
    (x, y) = _gsCPposNorth(ra_t_d, de_t_d, inr_d, mjd_end)
    x_inr_end = 90.0 - np.rad2deg(np.arctan2(y, x)) + inst_pa

    return _minorArc(x_inr_str, x_inr_end)

#! /bin/bash
set -e
set -u

#ARGS="--root $(pwd)/out"
ARGS="--sources deblended.fits --calexp calexp.fits --psf psf.fits"

# SDSS section
python examples/designdoc.py ${ARGS} --drillxy 1665,1543 --order 1,0,2 --pat "design-sdss-%(name)s.pdf"

# Monotonic section
python examples/designdoc.py ${ARGS} --drillxy 1545,2131 --pat "design-mono1-%(name)s.pdf" --figh 2.5
python examples/designdoc.py ${ARGS} --drillxy 1545,2131 --pat "design-mono2-%(name)s.pdf" --figh 2.5 --mono

python examples/designdoc.py ${ARGS} --drillxy 1378,3971 --order 3,0,1,2 --pat "design-mono3-%(name)s.pdf" --figw 3
python examples/designdoc.py ${ARGS} --drillxy 1378,3971 --order 3,0,1,2 --pat "design-mono4-%(name)s.pdf" --figw 3 --mono

# Median-filter section
python examples/designdoc.py ${ARGS} --drillxy 1545,2131 --pat "design-med1-%(name)s.pdf" --figh 2.5 --median
python examples/designdoc.py ${ARGS} --drillxy 1378,3971 --order 3,0,1,2 --pat "design-med2-%(name)s.pdf" --figw 3   --median


# Edge-ramp section
python examples/designdoc.py ${ARGS} --drillxy 10,158 --pat "design-ramp1-%(name)s.pdf" --figw 2 --median
python examples/designdoc.py ${ARGS} --drillxy 10,158 --pat "design-ramp2-%(name)s.pdf" --figw 2 --ramp

# Patch-edge section
python examples/designdoc.py ${ARGS} --drillxy 10,158 --pat "design-patch-%(name)s.pdf" --figw 2 --patch

# Ramp + Stray
python examples/designdoc.py ${ARGS} --drillxy 10,158 --pat "design-patch-%(name)s.pdf" --figw 2 --ramp2




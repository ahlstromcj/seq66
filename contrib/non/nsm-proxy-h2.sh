# Source: https://github.com/hydrogen-music/hydrogen/issues/32
#
# ..of course drumMachine.h2song has to be manually copied into the NSM
# Proxy.nXXXX directory in order for it to work, but once that's done you can
# duplicate the session and use it as a template. Not ideal, but it at least
# works for now! =D only other thing with that setup is I always save via NSM,
# then close Hydrogen, then abort the session, otherwise the .h2song will
# possibly get truncated/corrupted as mentioned above..

executable
    nohup hydrogen
arguments
    --nosplash --song ./drumMachine.h2song
save signal
    10
stop signal
    15
label
    Hydrogen

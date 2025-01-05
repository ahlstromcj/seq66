#!/bin/bash
#
# Chris Ahlstrom
# 2025-01-05 to 2025-01-05
#
# Adapted from https://github.com/bubbleguuum/toggle-monitor-grayscale
#
# Should also work with compositor=compton, untested.
#
#-----------------------------------------------------------------------

compositor=picom

function usage
{
    bin=$(basename $0)
    cat << E_O_F
Usage: grayscale [options]

Toggle monitors between color and grayscale mode via various mechanisms.

$bin [ $compositor | nvidia | ddc | auto]
$bin $compositor [$compositor args]
$bin nvidia [nv mon]"
$bin ddc [ddc mon]"

    $compositor:  Use a GLX shader to set grayscale.
    nvidia:  Use NVIDIA proprietary driver Digital Vibrance setting to
             set grayscale.
    ddc:     Use the DDC/CI monitor protocol to set the monitor saturation
             to 0 (grayscale) if supported by the monitor.
    auto:    Use $compositor if running, otherwise nvidia if available,
             otherwise ddc if available.

    $compositor args: in $compositor mode, optional $compositor parameters:

    nv mon:  In nvidia mode, 'mon' is an optional monitor name as enumerated
             by xrandr. If unspecified, apply to all monitors managed by
             the NVIDIA driver.
    ddc mon: In DDC mode, use optional ddcutil options to identify the
             monitor. See 'man ddcutil'.
             If unspecified, apply to the first monitor detected by ddcutil.
             If invoked with no argument, auto is used."
E_O_F

    exit 0
}

function toggle_nvidia
{
    dpy=$1
    value=$(nvidia-settings -t -q DigitalVibrance)
    desaturate_value=-1024

    # Set a value in the ]-1024..0[ range to desaturate colors instead
    # of full grayscale; -1024 => full grayscale.
    
    if (( value == $desaturate_value )); then
        value=0
        toggle_mode="color"
    else
        value=$desaturate_value
        toggle_mode="grayscale"
    fi

    if [ -n "$dpy" ]; then
        param="[DPY:$dpy]/DigitalVibrance"
    else
        param="DigitalVibrance"
    fi

    nvidia-settings -a ${param}=${value} > /dev/null
}

function toggle_compositor
{
    if $compositor --help | grep legacy-backends > /dev/null; then
        use_experimental_backends=1;
        grep_string="window-shader-fg"
    else
        use_experimental_backends=0;
        grep_string="glx-fshader-win"
    fi
    
    if pgrep -a -x $compositor | grep $grep_string  > /dev/null; then
        pkill -x $compositor
        sleep 1
        $compositor $* -b
        toggle_mode="color"
    else
        pkill -x $compositor
        sleep 1
        if (( $use_experimental_backends == 1 )); then
            tmpfile=$(mktemp)
            trap 'rm -f "${tmpfile}"' EXIT
            cat > ${tmpfile} << EOF
#version 330
in vec2 texcoord;
uniform sampler2D tex;
uniform float opacity;
vec4 default_post_processing(vec4 c);
vec4 window_shader()
{
    vec4 c = default_post_processing(texelFetch(tex, ivec2(texcoord), 0));
    float y = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
    c = opacity*vec4(y, y, y, c.a);
    return c;
}
EOF
	    
            $compositor $* -b --backend glx --window-shader-fg ${tmpfile} 2> /dev/null

        else

            shader='uniform sampler2D tex; uniform float opacity; void main() { vec4 c = texture2D(tex, gl_TexCoord[0].xy); float y = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722)); gl_FragColor = opacity*vec4(y, y, y, c.a); }'
            $compositor $* -b --backend glx --glx-fshader-win "${shader}" 2> /dev/null

        fi
	    
	toggle_mode="grayscale"

    fi
}

function toggle_ddc
{
    out=($(ddcutil $* getvcp 8a -t))
    
    if (( $? != 0 )); then
        echo "ddc: this monitor does not support saturation control"
        exit 1
    fi

    # out array:
    #
    # VCP 8A C 100 200
    #           |   |
    #          cur max
   
    if (( ${#out[@]} != 5 )); then
        echo "ddc: unexpected output getting current saturation state"
        exit 1
    fi

    cur_saturation=${out[3]}
    max_saturation=${out[4]}

    # Set a value in ]0..max/2[ range to desaturate colors instead of
    # full grayscale; # 0 => full grayscale.

    desaturate_value=0

    if (( cur_saturation == desaturate_value  )); then
        new_saturation=$(( max_saturation / 2 )) # nominal saturation 
        toggle_mode="color"
    else
        new_saturation=$desaturate_value
        toggle_mode="grayscale"
    fi

    ddcutil $* setvcp 8a $new_saturation
}

mode=$1

case $mode in

    --help | -h)

        usage
	;;

    $compositor)

        if ! pgrep -x $compositor > /dev/null; then
                echo "$compositor is not running"
                exit 1
        fi
	;;

    nvidia)

        if ! which nvidia-settings &> /dev/null; then
            echo "nvidia-settings is not installed"
            exit 1
        fi
	;;	   	

    ddc)

        if ! which ddcutil &> /dev/null; then 
            echo "ddcutil is not installed"
            exit 1
        fi
        ;;
    
    *)
        [ -z "$mode" ] && mode=auto
        if [ "$mode" = "auto" ]; then
            if pgrep -x $compositor > /dev/null; then
                mode=$compositor
            elif which nvidia-settings &> /dev/null; then
                mode=nvidia
            elif which ddcutil &> /dev/null; then
                mode=ddc
            else
                echo "Neither $compositor is running, nor is nvidia-settings"
                echo "installed, nor is ddcutil installed"
                exit 1
            fi
        else
            usage
        fi
	
esac

# Pass eventual remaining arguments to toggle_* function

if (( $# > 0 )); then
    shift
fi

if [ "$mode" = "nvidia" ]; then
    toggle_nvidia $*
elif [ "$mode" = "$compositor" ]; then
    toggle_compositor $*
else
    toggle_ddc $*
fi

if (( $? == 0 )); then
    echo "$mode: set to $toggle_mode"
else
    echo "$mode: toggle failed"
fi

#******************************************************************************
# grayscale.sh
#------------------------------------------------------------------------------
# vim: ts=4 sw=4 wm=4 et ft=sh
#------------------------------------------------------------------------------


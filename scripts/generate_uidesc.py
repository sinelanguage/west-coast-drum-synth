#!/usr/bin/env python3
"""Generate WestCoastEditor.uidesc matching the mockup pixel-perfectly.

Layout: 1024x530, 2-column x 4-row lane grid with compact global strip at top.
All positions and sizes are integer pixels for exact rendering.
"""

import xml.etree.ElementTree as ET

# ---------------------------------------------------------------------------
# Editor dimensions
# ---------------------------------------------------------------------------
EDITOR_W = 1024
EDITOR_H = 530

# ---------------------------------------------------------------------------
# Margins / header
# ---------------------------------------------------------------------------
LEFT_MARGIN = 12
TITLE_X = 14
TITLE_Y = 9
SUBTITLE_X = 500
SUBTITLE_Y = 10

# Global strip
GLOBAL_Y = 24
GLOBAL_H = 30
GLOBAL_W = 1000

# ---------------------------------------------------------------------------
# Lane grid
# ---------------------------------------------------------------------------
LANE_W = 494
LANE_H = 108
COL_GAP = 12
ROW_GAP = 6

COL0_X = LEFT_MARGIN          # 12
COL1_X = LEFT_MARGIN + LANE_W + COL_GAP  # 518

LANE_START_Y = 60  # after global strip
ROW_Y = [LANE_START_Y + i * (LANE_H + ROW_GAP) for i in range(4)]
# [60, 174, 288, 402]

SEP_Y = [ROW_Y[i] + LANE_H + 2 for i in range(3)]
# [170, 284, 398]

# ---------------------------------------------------------------------------
# Section layout inside a lane (all relative to lane origin)
# ---------------------------------------------------------------------------
ACCENT_W = 3
SEC_Y = 18      # section box top within lane
SEC_H = 82      # section box height
SEC_GAP = 1     # 1px line between section boxes

SEC_START_X = 7

# Section widths (expanded to fill space freed by tighter gaps)
SEC_WIDTHS = [96, 96, 116, 96, 72]
SEC_NAMES = ["OSC &amp; WAVESHAPE", "PITCH ENV", "TRANSIENT DESIGNER",
             "NOISE DESIGNER", "OUTPUT STAGE"]

# Compute section X positions
SEC_X = []
x = SEC_START_X
for w in SEC_WIDTHS:
    SEC_X.append(x)
    x += w + SEC_GAP

# Slider geometry within a section
SLIDER_W = 16
SLIDER_H = 46
SLIDER_STEP = 17  # 16px slider + 1px gap between bars
SLIDER_TOP = 15   # y offset within section (tight to title)
LABEL_TOP = 63    # y offset of label within section (1px below slider bottom)
LABEL_W = 16      # labels match slider width
LABEL_H = 10
SEC_TITLE_Y = 3
SEC_TITLE_H = 10

def slider_positions(sec_w, n):
    """Return (slider_x_list, label_x_list) for n sliders in section of width sec_w."""
    total_slider_span = (n - 1) * SLIDER_STEP + SLIDER_W
    margin = (sec_w - total_slider_span) // 2
    sx = [margin + i * SLIDER_STEP for i in range(n)]
    lx = [s for s in sx]
    return sx, lx

S4_96,  L4_96  = slider_positions(96, 4)   # OSC, PITCH, NOISE
S4_116, L4_116 = slider_positions(116, 4)  # TRANSIENT
S3_72,  L3_72  = slider_positions(72, 3)   # OUTPUT

SECTION_SLIDER_X = [S4_96, S4_96, S4_116, S4_96, S3_72]
SECTION_LABEL_X  = [L4_96, L4_96, L4_116, L4_96, L3_72]

# LED meter within lane
LED_X = 430
LED_Y = 8
LED_W = 54
LED_H = 6

# ---------------------------------------------------------------------------
# Lane definitions
# ---------------------------------------------------------------------------
LANE_COUNT = 9
VISIBLE_LANES = 8

LANE_PARAM_COUNT = 8
LANE_EXTRA_COUNT = 6
LANE_MACRO_COUNT = 4
LANE_FILTER_COUNT = 6

def core_tag(lane, offset):   return 100 + lane * LANE_PARAM_COUNT + offset
def extra_tag(lane, offset):  return 200 + lane * LANE_EXTRA_COUNT + offset
def macro_tag(lane, offset):  return 300 + lane * LANE_MACRO_COUNT + offset
def filter_tag(lane, offset): return 400 + lane * LANE_FILTER_COUNT + offset
def led_tag(lane):            return 500 + lane

LANE_NAMES = [
    ("DS1", "BD", "C"),
    ("DS2", "SD", "C#"),
    ("DS3", "LT", "D"),
    ("DS4", "MT", "D#"),
    ("DS5", "HT", "E"),
    ("DS6", "RS", "F"),
    ("DS7", "CP", "F#"),
    ("DS8", "HH", "G"),
    ("DS9", "CL", "G#"),
]

# Grid positions: (col, row) for each visible lane
LANE_GRID = [
    (0, 0), (0, 1), (0, 2), (0, 3),  # DS1-DS4 left column
    (1, 0), (1, 1), (1, 2), (1, 3),  # DS5-DS8 right column
]

# Visible slider tags per section for lane i
def section_tags(lane):
    """Return list of 5 tuples: (tags_list, labels_list, center_draw_list) per section."""
    c = lambda o: core_tag(lane, o)
    e = lambda o: extra_tag(lane, o)
    m = lambda o: macro_tag(lane, o)
    f = lambda o: filter_tag(lane, o)
    return [
        # OSC & WAVESHAPE: Tune, Decay, Fold, FM
        ([c(0), c(1), c(2), c(3)],
         ["PIT", "DEC", "FLD", "FM"],
         [False, False, False, False]),
        # PITCH ENV: PitchEnvAmt, PitchEnvDecay, OscCutoff, OscReso
        ([e(0), e(1), f(0), f(1)],
         ["PIT", "PDC", "OCF", "ORS"],
         [False, False, False, False]),
        # TRANSIENT DESIGNER: TransientAtk, TransDecay, TransMix, TransCutoff
        ([e(2), m(0), m(1), f(3)],
         ["ATK", "TDC", "TMX", "TCF"],
         [False, False, False, False]),
        # NOISE DESIGNER: Noise, NoiseTone, NoiseDecay, NoiseResonance
        ([c(4), e(3), e(4), m(2)],
         ["NOS", "TON", "NDC", "NRS"],
         [False, True, False, False]),
        # OUTPUT STAGE: Drive, Level, Pan
        ([c(5), c(6), c(7)],
         ["DRV", "LVL", "PAN"],
         [False, False, True]),
    ]

def hidden_tags(lane):
    """Tags that need invisible 2x2 sliders (5 per lane)."""
    return [
        extra_tag(lane, 5),   # Snap
        macro_tag(lane, 3),   # NoiseEnvAmount
        filter_tag(lane, 2),  # OscFltEnv
        filter_tag(lane, 4),  # TransReso
        filter_tag(lane, 5),  # TransFltEnv
    ]

# ---------------------------------------------------------------------------
# Control-tag names (for the <control-tags> section)
# ---------------------------------------------------------------------------
GLOBAL_TAG_NAMES = {
    0: "Master", 1: "InternalTempo", 2: "Swing",
    3: "Run", 4: "FollowHost", 5: "Preset", 6: "Randomize",
    7: "OscFilterCutoff", 8: "OscFilterResonance", 9: "OscFilterEnv",
}

LANE_PREFIXES = ["Kick", "Snare", "Hat", "Perc", "PercB",
                 "PercA2", "PercB2", "RimShot", "Clap"]
CORE_SUFFIXES = ["Tune", "Decay", "Fold", "Fm", "Noise", "Drive", "Level", "Pan"]
EXTRA_SUFFIXES = ["PitchEnv", "PitchEnvDecay", "Transient", "NoiseTone", "NoiseDecay", "Snap"]
MACRO_SUFFIXES = ["TransientDecay", "TransientMix", "NoiseResonance", "NoiseEnvAmount"]
FILTER_SUFFIXES = ["OscCutoff", "OscReso", "OscFltEnv", "TransCutoff", "TransReso", "TransFltEnv"]
LED_SUFFIX = "Led"

LED_NAMES = ["KickLed", "SnareLed", "HatLed", "PercALed", "PercA2Led",
             "PercBLed", "PercB2Led", "RimShotLed", "ClapLed"]

# ---------------------------------------------------------------------------
# XML generation helpers
# ---------------------------------------------------------------------------
def ind(level):
    return "  " * level

def slider_xml(tag, x, y, w, h, orientation, *, from_center=False, mouse=True,
               draw_back=True, draw_value=True, back_color="BarTrack",
               value_color="BarFill", opacity="1", transparent=False,
               wheel_inc="0.02", zoom="6"):
    attrs = (
        f'background-offset="0, 0" class="CSlider" control-tag="{tag}" '
        f'default-value="0.5" '
        f'draw-back="{str(draw_back).lower()}" draw-back-color="{back_color}" '
        f'draw-frame="false" draw-frame-color="{back_color}" '
        f'draw-value="{str(draw_value).lower()}" draw-value-color="{value_color}" '
        f'draw-value-from-center="{str(from_center).lower()}" draw-value-inverted="false" '
        f'handle-offset="0, 0" max-value="1" min-value="0" mode="free click" '
        f'mouse-enabled="{str(mouse).lower()}" opacity="{opacity}" '
        f'orientation="{orientation}" origin="{x}, {y}" reverse-orientation="false" '
        f'size="{w}, {h}" transparent="{str(transparent).lower()}" '
        f'transparent-handle="true" wheel-inc-value="{wheel_inc}" zoom-factor="{zoom}"'
    )
    return f'<view {attrs}/>'

def label_xml(x, y, w, h, text, font="label_micro", color="TextDim",
              bg="StageOuter", align="center"):
    return (
        f'<view back-color="{bg}" background-offset="0, 0" class="CTextLabel" '
        f'font="{font}" font-antialias="true" font-color="{color}" frame-width="0" '
        f'mouse-enabled="false" opacity="1" origin="{x}, {y}" size="{w}, {h}" '
        f'style-no-frame="true" text-alignment="{align}" title="{text}" transparent="true"/>'
    )

def section_title_label(x, y, w, text):
    return label_xml(x, y, w, SEC_TITLE_H, text, color="TextSubtle")

def cview_open(x, y, w, h, bg_color):
    return (
        f'<view background-color="{bg_color}" '
        f'background-color-draw-style="filled and stroked" '
        f'class="CViewContainer" mouse-enabled="true" opacity="1" '
        f'origin="{x}, {y}" size="{w}, {h}" transparent="false">'
    )

def cview_close():
    return '</view>'

def separator_line(x, y, w):
    return (
        f'<view background-color="RowLine" '
        f'background-color-draw-style="filled and stroked" '
        f'class="CView" mouse-enabled="false" opacity="1" '
        f'origin="{x}, {y}" size="{w}, 1" transparent="false"/>'
    )

def hidden_slider(tag, x, y):
    return (
        f'<view background-offset="0, 0" class="CSlider" control-tag="{tag}" '
        f'default-value="0.5" draw-back="false" draw-frame="false" draw-value="false" '
        f'handle-offset="0, 0" max-value="1" min-value="0" mode="free click" '
        f'mouse-enabled="true" opacity="0" orientation="vertical" '
        f'origin="{x}, {y}" reverse-orientation="false" size="2, 2" '
        f'transparent="true" transparent-handle="true" '
        f'wheel-inc-value="0.02" zoom-factor="1"/>'
    )

def led_slider(tag, x, y, w, h):
    return (
        f'<view background-offset="0, 0" class="CSlider" control-tag="{tag}" '
        f'default-value="0" draw-back="true" draw-back-color="BarTrack" '
        f'draw-frame="false" draw-frame-color="BarTrack" draw-value="true" '
        f'draw-value-color="AccentRed" draw-value-from-center="false" '
        f'draw-value-inverted="false" handle-offset="0, 0" max-value="1" '
        f'min-value="0" mode="free click" mouse-enabled="false" opacity="1" '
        f'orientation="horizontal" origin="{x}, {y}" reverse-orientation="false" '
        f'size="{w}, {h}" transparent="false" transparent-handle="true" '
        f'wheel-inc-value="0" zoom-factor="2"/>'
    )

def button_xml(tag, x, y, w, h, title):
    return (
        f'<view back-color="ButtonBase" background-offset="0, 0" '
        f'class="CTextButton" control-tag="{tag}" default-value="0" '
        f'font="label_micro" font-antialias="true" font-color="TextBright" '
        f'frame-color="ButtonFrame" frame-width="1" max-value="1" min-value="0" '
        f'mouse-enabled="true" opacity="1" origin="{x}, {y}" round-rect-radius="2" '
        f'size="{w}, {h}" style-round-rect="true" title="{title}" transparent="false"/>'
    )

def dropdown_xml(tag, x, y, w, h):
    return (
        f'<view back-color="ButtonBase" background-offset="0, 0" '
        f'class="COptionMenu" control-tag="{tag}" default-value="0" '
        f'font="label_micro" font-antialias="true" font-color="TextBright" '
        f'frame-color="ButtonFrame" frame-width="1" max-value="4" '
        f'menu-check-style="true" menu-popup-style="true" min-value="0" '
        f'mouse-enabled="true" opacity="1" origin="{x}, {y}" round-rect-radius="2" '
        f'size="{w}, {h}" style-round-rect="true" text-alignment="center" '
        f'transparent="false"/>'
    )

# ---------------------------------------------------------------------------
# Build lane module XML
# ---------------------------------------------------------------------------
def build_lane(lane_idx, col, row):
    lx = COL0_X if col == 0 else COL1_X
    ly = ROW_Y[row]
    ds_num, short_name, note = LANE_NAMES[lane_idx]
    lines = []
    d = ind(2)

    # Lane container
    lines.append(f'{ind(2)}{cview_open(lx, ly, LANE_W, LANE_H, "ModuleBg")}')

    # Accent bar
    lines.append(f'{ind(3)}<view background-color="AccentRed" '
                 f'background-color-draw-style="filled and stroked" '
                 f'class="CView" mouse-enabled="false" opacity="1" '
                 f'origin="0, 0" size="{ACCENT_W}, {LANE_H}" transparent="false"/>')

    # Lane title: "DS1" in white, "BD" in red
    lines.append(f'{ind(3)}{label_xml(9, 5, 24, 12, ds_num, font="label_title", color="TextBright", bg="ModuleBg", align="left")}')
    lines.append(f'{ind(3)}{label_xml(34, 5, 24, 12, short_name, font="label_title", color="AccentRed", bg="ModuleBg", align="left")}')

    # Note name
    lines.append(f'{ind(3)}{label_xml(60, 6, 20, 12, note, font="label_tiny", color="TextSubtle", bg="ModuleBg", align="left")}')

    # LED meter
    lines.append(f'{ind(3)}{led_slider(led_tag(lane_idx), LED_X, LED_Y, LED_W, LED_H)}')

    # 5 sections
    sects = section_tags(lane_idx)
    for si, (tags, labels, centers) in enumerate(sects):
        sx = SEC_X[si]
        sw = SEC_WIDTHS[si]
        slider_xs = SECTION_SLIDER_X[si]
        label_xs = SECTION_LABEL_X[si]

        lines.append(f'{ind(3)}{cview_open(sx, SEC_Y, sw, SEC_H, "StageOuter")}')

        # Section title
        title_w = sw - 4
        lines.append(f'{ind(4)}{section_title_label(2, SEC_TITLE_Y, title_w, SEC_NAMES[si])}')

        # Sliders and labels
        for j, (tag, lbl, fc) in enumerate(zip(tags, labels, centers)):
            sx_j = slider_xs[j]
            lx_j = label_xs[j]
            lines.append(f'{ind(4)}{slider_xml(tag, sx_j, SLIDER_TOP, SLIDER_W, SLIDER_H, "vertical", from_center=fc)}')
            lines.append(f'{ind(4)}{label_xml(lx_j, LABEL_TOP, LABEL_W, LABEL_H, lbl)}')

        lines.append(f'{ind(3)}{cview_close()}')

    lines.append(f'{ind(2)}{cview_close()}')
    return '\n'.join(lines)

# ---------------------------------------------------------------------------
# Build global strip XML
# ---------------------------------------------------------------------------
def build_global_strip():
    lines = []
    gx = LEFT_MARGIN
    gy = GLOBAL_Y
    gw = GLOBAL_W
    gh = GLOBAL_H

    lines.append(f'{ind(2)}{cview_open(gx, gy, gw, gh, "ModuleBg")}')
    lines.append(f'{ind(3)}<view background-color="AccentRed" '
                 f'background-color-draw-style="filled and stroked" '
                 f'class="CView" mouse-enabled="false" opacity="1" '
                 f'origin="0, 0" size="{ACCENT_W}, {gh}" transparent="false"/>')
    lines.append(f'{ind(3)}{label_xml(9, 4, 50, 12, "GLOBAL", font="label_title", color="TextBright", bg="ModuleBg", align="left")}')

    # CLOCK section (TMP, SWG) - tags 1, 2
    cx = 64
    lines.append(f'{ind(3)}{cview_open(cx, 3, 62, 24, "StageOuter")}')
    lines.append(f'{ind(4)}{section_title_label(2, 1, 58, "CLOCK")}')
    lines.append(f'{ind(4)}{slider_xml(1, 12, 10, 6, 12, "vertical", wheel_inc="0.02", zoom="4")}')
    lines.append(f'{ind(4)}{slider_xml(2, 36, 10, 6, 12, "vertical", wheel_inc="0.02", zoom="4")}')
    lines.append(f'{ind(3)}{cview_close()}')

    # MASTER section (LVL) - tag 0
    lines.append(f'{ind(3)}{cview_open(132, 3, 38, 24, "StageOuter")}')
    lines.append(f'{ind(4)}{section_title_label(2, 1, 34, "MASTER")}')
    lines.append(f'{ind(4)}{slider_xml(0, 15, 10, 6, 12, "vertical", wheel_inc="0.02", zoom="4")}')
    lines.append(f'{ind(3)}{cview_close()}')

    # BODY FILTER section (CUT, RES, ENV) - tags 7, 8, 9
    lines.append(f'{ind(3)}{cview_open(176, 3, 82, 24, "StageOuter")}')
    lines.append(f'{ind(4)}{section_title_label(2, 1, 78, "BODY FILTER")}')
    lines.append(f'{ind(4)}{slider_xml(7, 12, 10, 6, 12, "vertical", wheel_inc="0.02", zoom="4")}')
    lines.append(f'{ind(4)}{slider_xml(8, 34, 10, 6, 12, "vertical", wheel_inc="0.02", zoom="4")}')
    lines.append(f'{ind(4)}{slider_xml(9, 56, 10, 6, 12, "vertical", wheel_inc="0.02", zoom="4")}')
    lines.append(f'{ind(3)}{cview_close()}')

    # TRANSPORT section (RUN, FOLLOW, RANDOMIZE) - tags 3, 4, 6
    lines.append(f'{ind(3)}{cview_open(264, 3, 146, 24, "StageOuter")}')
    lines.append(f'{ind(4)}{section_title_label(2, 1, 142, "TRANSPORT")}')
    lines.append(f'{ind(4)}{button_xml(3, 4, 11, 30, 12, "RUN")}')
    lines.append(f'{ind(4)}{button_xml(4, 38, 11, 50, 12, "FOLLOW")}')
    lines.append(f'{ind(4)}{button_xml(6, 92, 11, 50, 12, "RANDOM")}')
    lines.append(f'{ind(3)}{cview_close()}')

    # PRESET section - tag 5
    lines.append(f'{ind(3)}{cview_open(416, 3, 110, 24, "StageOuter")}')
    lines.append(f'{ind(4)}{section_title_label(2, 1, 106, "PRESET")}')
    lines.append(f'{ind(4)}{dropdown_xml(5, 6, 11, 98, 12)}')
    lines.append(f'{ind(3)}{cview_close()}')

    lines.append(f'{ind(2)}{cview_close()}')
    return '\n'.join(lines)

# ---------------------------------------------------------------------------
# Build hidden sliders for non-visible params
# ---------------------------------------------------------------------------
def build_hidden_sliders():
    lines = []
    hx, hy = 990, 2
    idx = 0
    for lane in range(LANE_COUNT):
        for tag in hidden_tags(lane):
            lines.append(f'{ind(2)}{hidden_slider(tag, hx + idx % 10, hy + idx // 10)}')
            idx += 1
        # Also hide all params for lane 8 (visible lanes are 0-7)
        if lane == 8:
            # Lane 8 has ALL params hidden (core + extra + macro + filter + LED)
            for off in range(LANE_PARAM_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(core_tag(lane, off), hx + idx % 10, hy + idx // 10)}')
                idx += 1
            for off in range(LANE_EXTRA_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(extra_tag(lane, off), hx + idx % 10, hy + idx // 10)}')
                idx += 1
            for off in range(LANE_MACRO_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(macro_tag(lane, off), hx + idx % 10, hy + idx // 10)}')
                idx += 1
            for off in range(LANE_FILTER_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(filter_tag(lane, off), hx + idx % 10, hy + idx // 10)}')
                idx += 1
            lines.append(f'{ind(2)}{hidden_slider(led_tag(lane), hx + idx % 10, hy + idx // 10)}')
            idx += 1
    return '\n'.join(lines)

# ---------------------------------------------------------------------------
# Build control-tags section
# ---------------------------------------------------------------------------
def build_control_tags():
    lines = []
    lines.append(f'{ind(1)}<control-tags>')
    for tag, name in sorted(GLOBAL_TAG_NAMES.items()):
        lines.append(f'{ind(2)}<control-tag name="{name}" tag="{tag}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(CORE_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{core_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(EXTRA_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{extra_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(MACRO_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{macro_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(FILTER_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{filter_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        lines.append(f'{ind(2)}<control-tag name="{LED_NAMES[lane]}" tag="{led_tag(lane)}"/>')
    lines.append(f'{ind(1)}</control-tags>')
    return '\n'.join(lines)

# ---------------------------------------------------------------------------
# Main output
# ---------------------------------------------------------------------------
def generate():
    parts = []

    # Header
    parts.append('<?xml version="1.0" encoding="UTF-8"?>')
    parts.append('<vstgui-ui-description version="1">')

    # Fonts
    parts.append(f'{ind(1)}<fonts>')
    parts.append(f'{ind(2)}<font name="label_title" font-name="Arial Narrow" '
                 f'alternative-font-names="Helvetica Neue Condensed,Helvetica Neue,Nimbus Sans Narrow,Arial" '
                 f'size="10" bold="true" italic="false"/>')
    parts.append(f'{ind(2)}<font name="label_tiny" font-name="Arial Narrow" '
                 f'alternative-font-names="Helvetica Neue Condensed,Helvetica Neue,Nimbus Sans Narrow,Arial" '
                 f'size="8" bold="false" italic="false"/>')
    parts.append(f'{ind(2)}<font name="label_micro" font-name="Arial Narrow" '
                 f'alternative-font-names="Helvetica Neue Condensed,Helvetica Neue,Nimbus Sans Narrow,Arial" '
                 f'size="7" bold="false" italic="false"/>')
    parts.append(f'{ind(1)}</fonts>')

    # Colors
    parts.append(f'{ind(1)}<colors>')
    colors = [
        ("Backdrop", "#000000ff"), ("ModuleBg", "#04070bff"),
        ("StageOuter", "#4a4d53ff"), ("BarTrack", "#2c2f36ff"),
        ("BarFill", "#d3d5d8ff"), ("TextBright", "#f5f6f7ff"),
        ("TextDim", "#a6abb2ff"), ("TextSubtle", "#bcc1c6aa"),
        ("AccentRed", "#81222dff"), ("RowLine", "#1f242aff"),
        ("ButtonBase", "#252a30ff"), ("ButtonFrame", "#71767dff"),
    ]
    for name, rgba in colors:
        parts.append(f'{ind(2)}<color name="{name}" rgba="{rgba}"/>')
    parts.append(f'{ind(1)}</colors>')

    # Template
    parts.append(f'')
    parts.append(f'{ind(1)}<template background-color="Backdrop" '
                 f'background-color-draw-style="filled and stroked" '
                 f'class="CViewContainer" mouse-enabled="true" name="Editor" '
                 f'opacity="1" origin="0, 0" size="{EDITOR_W}, {EDITOR_H}" '
                 f'transparent="false" wants-focus="false">')

    # Title
    parts.append(f'{ind(2)}{label_xml(TITLE_X, TITLE_Y, 100, 14, "WSDS-F416", font="label_title", color="TextBright", bg="Backdrop", align="left")}')

    # Subtle binary text
    parts.append(f'{ind(2)}{label_xml(SUBTITLE_X, SUBTITLE_Y, 76, 10, "0011011010", font="label_micro", color="TextSubtle", bg="Backdrop", align="center")}')

    # Global strip
    parts.append(build_global_strip())

    # Separator line below global strip
    sep_w = COL1_X + LANE_W - LEFT_MARGIN
    parts.append(f'{ind(2)}{separator_line(LEFT_MARGIN, LANE_START_Y - 2, sep_w)}')

    # Row separator lines between lane rows
    for sy in SEP_Y:
        parts.append(f'{ind(2)}{separator_line(LEFT_MARGIN, sy, sep_w)}')

    # Visible lanes (0-7)
    for vi in range(VISIBLE_LANES):
        col, row = LANE_GRID[vi]
        parts.append(build_lane(vi, col, row))

    # Hidden sliders
    parts.append(build_hidden_sliders())

    # Close template
    parts.append(f'{ind(1)}</template>')

    # Control tags
    parts.append('')
    parts.append(build_control_tags())

    # Gradients
    parts.append(f'')
    parts.append(f'{ind(1)}<gradients>')
    parts.append(f'{ind(2)}<gradient name="Switch On">')
    parts.append(f'{ind(3)}<color-stop rgba="#00d4aaff" start="0"/>')
    parts.append(f'{ind(3)}<color-stop rgba="#00997aff" start="1"/>')
    parts.append(f'{ind(2)}</gradient>')
    parts.append(f'{ind(2)}<gradient name="Switch On Active">')
    parts.append(f'{ind(3)}<color-stop rgba="#4da6ffff" start="0"/>')
    parts.append(f'{ind(3)}<color-stop rgba="#2d7accff" start="1"/>')
    parts.append(f'{ind(2)}</gradient>')
    parts.append(f'{ind(2)}<gradient name="Switch Off">')
    parts.append(f'{ind(3)}<color-stop rgba="#4a4f58ff" start="0"/>')
    parts.append(f'{ind(3)}<color-stop rgba="#3a3e46ff" start="1"/>')
    parts.append(f'{ind(2)}</gradient>')
    parts.append(f'{ind(1)}</gradients>')
    parts.append('</vstgui-ui-description>')

    return '\n'.join(parts)

if __name__ == '__main__':
    import sys
    output = generate()
    out_path = sys.argv[1] if len(sys.argv) > 1 else 'resource/WestCoastEditor.uidesc'
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write(output)
        f.write('\n')
    print(f"Generated {out_path} ({len(output)} chars)")

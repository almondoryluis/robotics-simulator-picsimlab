/* ########################################################################

   PICsimLab - PIC laboratory simulator

   ########################################################################

   Copyright (c) : 2010-2022  Luis Claudio Gambôa Lopes

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   For e-mail suggestions :  lcgamboa@yahoo.com
   ######################################################################## */

#include "input_encoder.h"
#include "../oscilloscope.h"
#include "../picsimlab.h"
#include "../spareparts.h"

#define dprintf \
    if (1) {    \
    } else      \
        printf

/* outputs */
enum { O_RT1, O_BTN, O_P1, O_P2, O_P3 };

/* inputs */
enum { I_RT1, I_BTN };

static PCWProp pcwprop[5] = {{PCW_LABEL, "1 - VCC,+5V"},
                             {PCW_LABEL, "2 - GND ,GND"},
                             {PCW_COMBO, "3 - OA"},
                             {PCW_COMBO, "4 - OB"},
                             {PCW_COMBO, "5 - BTN"}};

cpart_encoder::cpart_encoder(const unsigned x, const unsigned y, const char* name, const char* type)
    : part(x, y, name, type), font(9, lxFONTFAMILY_TELETYPE, lxFONTSTYLE_NORMAL, lxFONTWEIGHT_BOLD) {
    always_update = 1;

    p_BTN = 1;

    output_pins[0] = 0;
    output_pins[1] = 0;
    output_pins[2] = 0;

    value = 0;
    value_old = 0;

    active = 0;

    count = 0;
    state = 0;

    SetPCWProperties(pcwprop, 5);
}

void cpart_encoder::RegisterRemoteControl(void) {
    input_ids[I_RT1]->status = &value;
    input_ids[I_RT1]->update = &output_ids[O_RT1]->update;
    input_ids[I_BTN]->status = &p_BTN;
    input_ids[I_BTN]->update = &output_ids[O_BTN]->update;
}

cpart_encoder::~cpart_encoder(void) {
    delete Bitmap;
    canvas.Destroy();
}

void cpart_encoder::DrawOutput(const unsigned int i) {
    int x, y;

    switch (output[i].id) {
        case O_P1:
        case O_P2:
        case O_P3:
            canvas.SetColor(49, 61, 99);
            canvas.Rectangle(1, output[i].x1, output[i].y1, output[i].x2 - output[i].x1, output[i].y2 - output[i].y1);
            canvas.SetFgColor(255, 255, 255);
            if (output_pins[output[i].id - O_P1] == 0)
                canvas.RotatedText("NC", output[i].x1 - 3, output[i].y2, 90);
            else
                canvas.RotatedText(SpareParts.GetPinName(output_pins[output[i].id - O_P1]), output[i].x1 - 3,
                                   output[i].y2, 90);
            break;
        case O_RT1:
            canvas.SetColor(102, 102, 102);
            canvas.Rectangle(1, output[i].x1, output[i].y1, output[i].x2 - output[i].x1, output[i].y2 - output[i].y1);

            canvas.SetFgColor(0, 0, 0);
            canvas.SetBgColor(44, 44, 44);
            canvas.Circle(1, output[i].cx, output[i].cy, 25);

            canvas.SetBgColor(250, 250, 250);
            x = -18 * sin((2 * M_PI * (value / 200.0)));
            y = 18 * cos((2 * M_PI * (value / 200.0)));
            canvas.Circle(1, output[i].cx + x, output[i].cy + y, 5);
            break;
        case O_BTN:
            if (p_BTN) {
                canvas.SetColor(77, 77, 77);
            } else {
                canvas.SetColor(22, 22, 22);
            }
            canvas.Circle(1, output[i].cx, output[i].cy, 9);
            break;
    }
}

void cpart_encoder::PreProcess(void) {
    unsigned char value_ = value;
    float da;

    value_ = value;

    da = (value_ / 2.5) - (value_old / 2.5);

    if (da < -40) {
        da = ((value_ + 200) / 2.5) - (value_old / 2.5);
    }
    if (da > 40) {
        da = ((value_ - 200) / 2.5) - (value_old / 2.5);
    }

    if (da != 0) {
        dir = ((da > 0) ? 1 : 0);

        if (da == 0) {
            step = 0;
        } else if (fabs(da) < 1.0) {
            state = (((value_old) % 10) * 10) / 25;

            switch (state) {
                case 0:
                    if (output_pins[0])
                        SpareParts.SetPin(output_pins[0], 0);
                    break;
                case 1:
                    if (output_pins[1])
                        SpareParts.SetPin(output_pins[1], 1);
                    break;
                case 2:
                    if (output_pins[0])
                        SpareParts.SetPin(output_pins[0], 1);
                    break;
                case 3:
                    if (output_pins[1])
                        SpareParts.SetPin(output_pins[1], 0);
                    break;
            }
            step = 0;
            // FIXME on slow speed output is not 90 degrees
        } else {
            step = PICSimLab.GetBoard()->MGetInstClockFreq() / ((da > 0) ? da * 10 : -da * 10);
        }

        dprintf("state=%i da=%f  %3i  %3i  dir=%i step=%i\n", state, da, value_, value_old, dir, step);

        value_old = value_;
    } else {
        step = 0;
    }

    if (output_pins[2])
        SpareParts.SetPin(output_pins[2], p_BTN);
}

void cpart_encoder::Process(void) {
    if (step) {
        count++;

        if (count >= step) {
            count = 0;

            if (dir) {
                state++;
                if (state > 3)
                    state = 0;

            } else {
                state--;
                if (state < 0)
                    state = 3;
            }

            dprintf("state=%i\n", state);
            switch (state) {
                case 0:
                    if (output_pins[0])
                        SpareParts.SetPin(output_pins[0], 0);
                    break;
                case 1:
                    if (output_pins[1])
                        SpareParts.SetPin(output_pins[1], 1);
                    break;
                case 2:
                    if (output_pins[0])
                        SpareParts.SetPin(output_pins[0], 1);
                    break;
                case 3:
                    if (output_pins[1])
                        SpareParts.SetPin(output_pins[1], 0);
                    break;
            }
        }
    }
}

void cpart_encoder::OnMouseButtonPress(uint inputId, uint button, uint x, uint y, uint state) {
    switch (inputId) {
        case I_RT1:
            if (p_BTN) {
                value = CalcAngle(inputId, x, y);
                active = 1;
                output_ids[O_BTN]->update = 1;
                output_ids[O_RT1]->update = 1;
            }
            break;
        case I_BTN:
            p_BTN = 0;
            output_ids[O_BTN]->update = 1;
            break;
    }
}

void cpart_encoder::OnMouseButtonRelease(uint inputId, uint button, uint x, uint y, uint state) {
    switch (inputId) {
        case I_RT1:
            active = 0;
            output_ids[O_BTN]->update = 1;
            output_ids[O_RT1]->update = 1;
            break;
        case I_BTN:
            p_BTN = 1;
            output_ids[O_BTN]->update = 1;
            break;
    }
}

void cpart_encoder::OnMouseMove(uint inputId, uint button, uint x, uint y, uint state) {
    switch (inputId) {
        case I_RT1:
            if (active) {
                value = CalcAngle(inputId, x, y);
                output_ids[O_BTN]->update = 1;
                output_ids[O_RT1]->update = 1;
            }
            break;
        default:
            active = 0;
            break;
    }
}

unsigned char cpart_encoder::CalcAngle(int inputId, int x, int y) {
    int dx = input_ids[inputId]->cx - x;
    int dy = y - input_ids[inputId]->cy;
    double angle = 0;

    if ((dx >= 0) && (dy >= 0)) {
        angle = atan2(dx, dy) * 180 / M_PI;
    } else if ((dx >= 0) && (dy < 0)) {
        angle = 180 - (atan2(dx, -dy) * 180 / M_PI);
    } else if ((dx < 0) && (dy < 0)) {
        angle = (atan2(-dx, -dy) * 180 / M_PI) + 180;
    } else if ((dx < 0) && (dy >= 0)) {
        angle = 360 - (atan2(-dx, dy) * 180 / M_PI);
    }

    return (200 * angle / 360.0);
}

unsigned short cpart_encoder::GetInputId(char* name) {
    if (strcmp(name, "RT_1") == 0)
        return I_RT1;
    if (strcmp(name, "PB_1") == 0)
        return I_BTN;

    printf("Erro input '%s' don't have a valid id! \n", name);
    return -1;
}

unsigned short cpart_encoder::GetOutputId(char* name) {
    if (strcmp(name, "PN_1") == 0)
        return O_P1;
    if (strcmp(name, "PN_2") == 0)
        return O_P2;
    if (strcmp(name, "PN_3") == 0)
        return O_P3;

    if (strcmp(name, "RT_1") == 0)
        return O_RT1;
    if (strcmp(name, "PB_1") == 0)
        return O_BTN;

    printf("Erro output '%s' don't have a valid id! \n", name);
    return 1;
}

lxString cpart_encoder::WritePreferences(void) {
    char prefs[256];

    sprintf(prefs, "%hhu,%hhu,%hhu,%hhu", output_pins[0], output_pins[1], output_pins[2], value);

    return prefs;
}

void cpart_encoder::ReadPreferences(lxString value) {
    sscanf(value.c_str(), "%hhu,%hhu,%hhu,%hhu", &output_pins[0], &output_pins[1], &output_pins[2], &this->value);

    value_old = this->value;
}

void cpart_encoder::ConfigurePropertiesWindow(CPWindow* WProp) {
    SetPCWComboWithPinNames(WProp, "combo3", output_pins[0]);
    SetPCWComboWithPinNames(WProp, "combo4", output_pins[1]);
    SetPCWComboWithPinNames(WProp, "combo5", output_pins[2]);
}

void cpart_encoder::ReadPropertiesWindow(CPWindow* WProp) {
    output_pins[0] = GetPWCComboSelectedPin(WProp, "combo3");
    output_pins[1] = GetPWCComboSelectedPin(WProp, "combo4");
    output_pins[2] = GetPWCComboSelectedPin(WProp, "combo5");
}

part_init(PART_ENCODER_Name, cpart_encoder, "Input");
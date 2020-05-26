/*
 * Copyright (c) 2020, Nick Vella <nick@nxk.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/JsonArray.h>
#include <AK/JsonObject.h>
#include <AK/JsonValue.h>
#include <AK/SharedBuffer.h>
#include <LibCore/File.h>
#include <LibCore/Timer.h>
#include <LibGUI/Application.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/Palette.h>
#include <stdio.h>
#include <time.h>

class NetworkWidget final : public GUI::Widget {
    C_OBJECT(NetworkWidget)
public:
    NetworkWidget(const String& interface)
    {
        m_interface = interface;

        m_net_disconnected_bitmap = Gfx::Bitmap::load_from_file("/res/icons/network/net-disconnect.png");
        m_net_up_bitmap = Gfx::Bitmap::load_from_file("/res/icons/network/net-up.png");
        m_net_down_bitmap = Gfx::Bitmap::load_from_file("/res/icons/network/net-down.png");
        m_net_updown_bitmap = Gfx::Bitmap::load_from_file("/res/icons/network/net-updown.png");
        m_net_idle_bitmap = Gfx::Bitmap::load_from_file("/res/icons/network/net-idle.png");

        m_timer = add<Core::Timer>(500, [this] {
            static time_t last_update_time;
            time_t now = time(nullptr);
            if (now != last_update_time) {
                tick();
                last_update_time = now;
            }
        });
    }

    virtual ~NetworkWidget() override {}

    int get_width()
    {
        return 16 + menubar_menu_margin();
    }

private:
    static int menubar_menu_margin() { return 2; }

    void tick()
    {
        auto file = Core::File::construct("/proc/net/adapters");
        if (!file->open(Core::IODevice::ReadOnly)) {
            fprintf(stderr, "Error: %s\n", file->error_string());
            return;
        }
        auto file_contents = file->read_all();
        auto json = JsonValue::from_string(file_contents).as_array();

        json.for_each([this](auto& value) {
            auto if_object = value.as_object();

            auto name = if_object.get("name").to_string();
            if(name != m_interface) return;
            
            // Divide by various kilobytes to only show 'big' network traffic rather than every TCP ACK
            auto bytes_down = if_object.get("bytes_in").to_u32();
            auto bytes_up = if_object.get("bytes_out").to_u32();

            u32 up_delta = bytes_up - m_bytes_up;
            u32 down_delta = bytes_down - m_bytes_down;

            dbgprintf("\tin\tout\nold\t%i\t%i\nnew\t%i\t%i\ndelta\t%i\t%i\n", m_bytes_down, m_bytes_up, bytes_down, bytes_up, up_delta, down_delta);

            // Blink if over 32KB is transferred in either direction
            int new_state = ((down_delta > 16000) << 1) | (up_delta > 16000);
            m_bytes_down = bytes_down;
            m_bytes_up = bytes_up;

            if(new_state != m_state) {
                m_state = new_state;
                update();
            }
        });
    }

    virtual void paint_event(GUI::PaintEvent& event) override
    {
        GUI::Painter painter(*this);
        painter.add_clip_rect(event.rect());
        painter.clear_rect(event.rect(), Color::from_rgba(0));

        Gfx::Bitmap& bitmap = *m_net_disconnected_bitmap;
        switch(m_state) {
            case 0b00:
                bitmap = *m_net_idle_bitmap;
                break;
            case 0b10:
                bitmap = *m_net_down_bitmap;
                break;
            case 0b11:
                bitmap = *m_net_updown_bitmap;
                break;
        }

        painter.blit({}, bitmap, bitmap.rect());
    }

    RefPtr<Gfx::Bitmap> m_net_disconnected_bitmap;
    RefPtr<Gfx::Bitmap> m_net_up_bitmap;
    RefPtr<Gfx::Bitmap> m_net_down_bitmap;
    RefPtr<Gfx::Bitmap> m_net_updown_bitmap;
    RefPtr<Gfx::Bitmap> m_net_idle_bitmap;

    RefPtr<Core::Timer> m_timer;
    
    int m_state = 0b00;
    String m_interface;
    u32 m_bytes_up = 0;
    u32 m_bytes_down = 0;
};

int main(int argc, char** argv)
{
    if (pledge("stdio shared_buffer accept rpath unix cpath fattr", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    GUI::Application app(argc, argv);

    if (pledge("stdio shared_buffer accept rpath", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    auto window = GUI::Window::construct();
    window->set_title("Network");
    window->set_window_type(GUI::WindowType::MenuApplet);
    window->set_has_alpha_channel(true);

    auto& widget = window->set_main_widget<NetworkWidget>("e1k0");
    window->resize(widget.get_width(), 16);
    window->show();

    if (unveil("/res", "r") < 0) {
        perror("unveil");
        return 1;
    }

    if (unveil("/proc/net/adapters", "r") < 0) {
        perror("unveil");
        return 1;
    }

    unveil(nullptr, nullptr);

    return app.exec();
}

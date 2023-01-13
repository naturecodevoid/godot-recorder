#include "recorder.h"
#include "Defs.hpp"
#include "GodotGlobal.hpp"
#include "Object.hpp"
#include "ProjectSettings.hpp"
#include "Thread.hpp"
#include "Variant.hpp"
#include "gdnative/gdnative.h"
#include "gdnative/string.h"
#include <Viewport.hpp>
#include <ViewportTexture.hpp>
#include <Image.hpp>
#include <CanvasItem.hpp>
#include <Transform.hpp>
#include <String.hpp>
#include <Label.hpp>
#include <GlobalConstants.hpp>
#include <Engine.hpp>
#include <SceneTree.hpp>
#include <OS.hpp>
#include <Directory.hpp>
#include <Timer.hpp>
#include <PoolArrays.hpp>
#include <stdint.h>
#include "mpeg_writer.h"

godot::String boolAsString(bool x) {
    if (x) return "true";
    return "false";
}

namespace godot {
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define GD_LINE godot::Godot::print(__FILE__ ": " LINE_STRING)
    void Recorder::_register_methods() {
        //methods
        register_method("_process", &Recorder::_process);
        register_method("_ready", &Recorder::_ready);
        register_method("_save_frame", &Recorder::_save_frame);
        register_method("_save_frames", &Recorder::_save_frames);
        register_method("record_duration", &Recorder::record_duration);
        register_method("toggle_record", &Recorder::toggle_record);
        register_method("save_timer_complete", &Recorder::_save_timer_complete);
        register_method("toggle_timer_complete", &Recorder::_toggle_timer_complete);
        //props
        register_property<Recorder, Label>("label", &Recorder::label, Label::_new());
        register_property<Recorder, float>("frames_per_second", &Recorder::frames_per_second, 60);
        register_property<Recorder, String>("output_folder", &Recorder::output_folder, ProjectSettings::get_singleton()->globalize_path("user://recorder_out"));
    }
    //#todo: lots of uninitialised vars here.
    Recorder::Recorder()
        : frames_per_second(60),
        output_folder(ProjectSettings::get_singleton()->globalize_path("user://recorder_out")),
        _viewport(nullptr),
        _thread(nullptr),
        _save_timer(nullptr),
        _toggle_timer(nullptr),
        use_thread(true),
        flip_y(true)
        // _writer(nullptr)
    {
    }
    Recorder::~Recorder() {
        _viewport = nullptr;
        // add your cleanup here
        if (_thread != nullptr) {
            if (_thread->is_active()) {
                _thread->wait_to_finish();
            }
            _thread->unreference();
        }
        // the rest of these are associated with the scene, might need to add refs if I want to clean them up here.
        // #todo: seems like these might be leaking, but they don't come with ref class accessors. Not sure what the clean-up pattern is.
        // if(label != nullptr)
        // {
        // 	 label->free();
        // 	 label = nullptr;
        // }
        // GD_LINE;
        // if(_save_timer != nullptr)
        // {
        // 	 _save_timer->free();
        // 	 _save_timer = nullptr;
        // }
        // GD_LINE;
        // if(_toggle_timer != nullptr)
        // {
        // 	 _toggle_timer->free();
        // 	 _toggle_timer = nullptr;
        // }
        // GD_LINE;
        // if(_writer != nullptr)
        // {
        // 	 delete _writer;
        // 	 _writer = nullptr;
        // }
    }
    void Recorder::_init() {}
    void Recorder::_ready() {
        // Initialize any variables here
        _viewport = get_viewport();
        fpsConstant = 1.0 / frames_per_second;
        _frametick = 0;
        _thread = Ref<Thread>(Thread::_new());
        _recordThread = Ref<Thread>(Thread::_new());
        label = Label::_new();
        // # If running on editor, DONT override process and input
        set_process(false);
        set_process_input(false);
        auto engine = Engine::get_singleton();
        if (!engine->is_editor_hint()) {
            set_process(true);
            set_process_input(true);
        }
        //# Some recorder info to show onscreen
        _viewport->call_deferred("add_child", label);
        //#		print(get_tree().root)
        //#		get_tree().root.add_child(label)
        //#		label.text = "Waiting for capture...\nNumber of frames recorded: " + str(_images.size())


        label->set_anchor(GlobalConstants::MARGIN_BOTTOM, 1);
        label->set_margin(GlobalConstants::MARGIN_BOTTOM, -10);
        label->set_anchor(GlobalConstants::MARGIN_TOP, 1);
        label->set_margin(GlobalConstants::MARGIN_TOP, -40);
        label->hide();


        _save_timer = Timer::_new();
        _toggle_timer = Timer::_new();
        _save_timer->connect("timeout", this, "save_timer_complete");
        _save_timer->set_one_shot(true);
        _toggle_timer->connect("timeout", this, "toggle_timer_complete");
        _toggle_timer->set_one_shot(true);
        _viewport->call_deferred("add_child", _save_timer);
        _viewport->call_deferred("add_child", _toggle_timer);
    }
    void Recorder::start_recording() {
        if (_running) {
            Godot::print_error("You can't start recording if you are already recording!");
            return;
        }

        _running = true;
        _recordThread->start(this, "_record");
        Godot::print("Started recording");
    }
    void Recorder::stop_recorder() {
        if (!_running) {
            Godot::print_error("You can't stop recording if you are not already recording!");
            return;
        }

        _running = false;
        Godot::print("Stopping recording");
        if (_recordThread->is_active()) {
            _recordThread->wait_to_finish();
        }
        Godot::print("Stopped recording");
    }
    void Recorder::_record() {
        while (_running) {
            if (_frametick > fpsConstant) {
                _frametick = 0;
                Godot::print("x " + String::num(_viewport->get_size().x) + " y " + String::num(_viewport->get_size().y));
                Godot::print("delta " + String::num(delta) + " _frametick " + String::num(_frametick));
                _images.append(_viewport->get_texture()->get_data());
            }
        }
    }
    void Recorder::_process(float delta) {
        if (_running) {
            _frametick += delta;
        }
    }
    void Recorder::_save_frame() {
        _images.append(_viewport->get_texture()->get_data());

    }
    void Recorder::toggle_record() {
        if (_thread->is_active() || _saving) {
            return;
        }
        _running = !_running;
        Godot::print("Toggled recording to " + boolAsString(_running));
        label->hide();
        if (!_running) {
            label->show();
            if (use_thread) {
                if (!_thread->is_active()) {
                    auto err = _thread->start(this, "_save_frames");
                }
            }
            else {
                _save_frames();
            }
        }
    }
    void Recorder::_save_frames() {
        _saving = true;
        //# userdata wont be used, is just for the thread calling
        String scene_name = get_tree()->get_current_scene()->get_name();
        Dictionary date_time = OS::get_singleton()->get_datetime();
        const String hour = date_time["hour"];
        const String minute = date_time["minute"];
        const String second = date_time["second"];
        // if(hour && minute && second)
        {
            String time_str = hour + "-" + minute + "-" + second;
            Directory* dir = Directory::_new();\
                dir->make_dir(output_folder);
            if (dir->open(output_folder) != Error::OK) {
                ERR_PRINT("An error occurred when trying to create the output folder.");
            }
            String writer_path = (output_folder + "/" + scene_name + "_" + time_str + ".mp4");
            Godot::print("Writing to " + writer_path);
            {
                const char* c_string = writer_path.alloc_c_string();
                mpeg_writer* writer = new mpeg_writer(c_string, _viewport->get_size().x, _viewport->get_size().y, frames_per_second);
                for (int i = 0; i < _images.size(); i++) {
                    Ref<Image> image = _images[i];
                    label->set_text("Saving frames...(" + String::num(i) + "/" + String::num(_images.size()) + ")");
                    image->crop(get_rect().size.x, get_rect().size.y);
                    if (flip_y) {
                        image->flip_y();
                    }
                    image->convert(Image::FORMAT_RGB8);
                    auto pool = image->get_data();
                    auto* ptr = pool.read().ptr();
                    writer->add_frame((uint8_t*)ptr);
                }
                Godot::print("Wrote " + String::num(_images.size()) + " frames");
                delete writer;
                writer = nullptr;
            }
            _images.clear();
            label->set_text("Done!");
            _save_timer->start(1);
            // Godot::print("wrote: "+writer_path);
        }
    }
    void Recorder::record_duration(float duration = 5) {
        if (_running) { return; }
        Godot::print("Recording for " + String::num(duration) + " seconds");
        toggle_record();
        _toggle_timer->start(duration);
    }
    void Recorder::_save_timer_complete() {
        label->set_text("");
        label->hide();
        _saving = false;
    }
    void Recorder::_toggle_timer_complete() {
        toggle_record();
    }
} // namespace godot

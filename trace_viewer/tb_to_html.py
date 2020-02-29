#!/usr/bin/python3

import argparse
import pathlib
import sys
import jinja2
import flask
import json


class Var:
    def __init__(self, ID, filepath, name, line_num):
        self.ID = ID
        self.filepath = filepath
        self.filename = filepath.name
        self.name = name
        self.line_num = line_num
        # print("New var ID:", self.ID)


class TableEntry:
    def __init__(self, time, val):
        self.time = time
        self.value = val
        self.next_entry = None


class Device:
    def __init__(self, device_id):
        self.device_id = device_id
        self.threads = []


class Thread:
    def __init__(self, thread_id, tb_size):
        self.thread_id = thread_id
        self.zero_index = None
        self.final_offset = None
        self.buffer = [] 
        self.entries = {}


def parse_tb():
    parser = argparse.ArgumentParser()
    parser.add_argument("tracebuffer_file",
                        help="Path to trace buffer data dump file")
    parser.add_argument("variable_database_file")
    args = parser.parse_args()

    tb = open(args.tracebuffer_file, 'r').read().split()
    tb = [int(i) for i in tb]

    i = 0
    num_devices = tb[i]
    i += 1
    print("num devices:", num_devices)

    num_threads = tb[i]
    i += 1
    print("num threads:", num_threads)

    tb_size = tb[i]
    i += 1
    print("TBSize:", tb_size)

    devices = []
    def current_thread(): return devices[-1].threads[-1]

    # Read trace buffer into thread buffers
    while i < len(tb):
        val = tb[i]
        i += 1

        if val >= 0x10000000:
            device_id = val & 0x0FFFFFFF
            
            device = Device(device_id)
            device.device_id = device_id
            devices.append(device)

            thread_idx = 0
            thread = Thread(thread_idx, tb_size)
            device.threads.append(thread)

            current_thread().thread_idx = thread_idx
        else:
            if len(current_thread().buffer) >= tb_size:
                thread_idx += 1
                thread = Thread(thread_idx, tb_size)
                devices[-1].threads.append(thread)

                if thread_idx == num_threads:
                    thread_idx = 0

            current_thread().thread_id = thread_idx
            next_val = tb[i]
            i += 1
            if val == 0:
                current_thread().zero_index = len(current_thread().buffer) #buffer_idx
                current_thread().final_offset = next_val

            current_thread().buffer.append(val)
            current_thread().buffer.append(next_val)

    # Sort buffers
    for device in devices:
        for thread in device.threads:
            final_offset = thread.final_offset
            zero_index = thread.zero_index

            if final_offset > tb_size:
                thread.buffer = thread.buffer[zero_index + 2:] + thread.buffer[:zero_index] + [0, 0]
            else:
                thread.buffer[final_offset:] = [0] * (tb_size - final_offset)

    # Create lists
    for device in devices:
        for thread in device.threads:
            k = 0
            while k < len(thread.buffer):
                ID = thread.buffer[k]
                if ID == 0:
                    k += 2
                    continue

                val = thread.buffer[k + 1]
                if ID not in thread.entries:
                    thread.entries[ID] = []

                thread.entries[ID].append(TableEntry(k, val))
                k += 2

    # Mapping to source files
    id_to_vars = {}
    new_func = True

    database_lines = open(args.variable_database_file).readlines()

    for line in database_lines:
        if new_func:
            line_split = line.split(":")
            cl_filepath = line_split[1]
            print("CLFileName:", cl_filepath)
            new_func = False
        else:
            line_split = line.split()
            for var_str in line_split:
                var_split = var_str.split(":")
                ID = var_split[2]
                if ID[-1] == ",":
                    ID = ID[:-1]
                ID = int(ID)
                var_name = var_split[0]
                line_num = int(var_split[1])
                var = Var(ID=ID, filepath=pathlib.Path(cl_filepath),
                          name=var_name, line_num=line_num)
                id_to_vars[ID] = var
    return (devices, id_to_vars)


def build_trace_json(devices, id_to_vars):
    static_path = pathlib.Path("static")
    data = {}       
    var_list = []
    data["variables"] = var_list

    v_obj_list = list(id_to_vars.values())
    v_obj_list.sort(key = lambda v: v.line_num)
    for v in v_obj_list:
        var_item = {}
        var_item["name"] = v.name
        var_item["ID"] = v.ID
        var_item["line_num"] = v.line_num
        var_list.append(var_item) 

    var_updates = []
    data["var_updates"] = var_updates
    for d in devices:
        for t in d.threads:
            for (ID, updates) in t.entries.items():
                for u in updates:
                    update = {}
                    update["ID"] = ID
                    update["thread"] = "(Device " + str(d.device_id) + ") Thread " + str(t.thread_id)
                    update["time"] = u.time
                    update["val"] = u.value
                    var_updates.append(update)
    var_updates.sort(key = lambda u: u["time"])


    with open(static_path / "trace.js", 'w') as fp:
        fp.write("trace_data = ")
        json.dump(data, fp)

APP = flask.Flask(__name__)


@APP.route('/')
def index():
    devices = APP.config["devices"]
    device_names = ["Device " + str(d.device_id) for d in devices]
    print(device_names)

    id_to_vars = APP.config["id_to_vars"]
    var_list = id_to_vars.values()

    thread_names = list(set([ "(Device " +
                             str(d.device_id) + ") Thread " + str(t.thread_id)  for d in devices for t in d.threads]))
    return flask.render_template('index.html', device_names=device_names, thread_names=thread_names, var_list = var_list)


def run_html_server(devices, id_to_vars):
    APP.debug = True
    APP.config["devices"] = devices
    APP.config["id_to_vars"] = id_to_vars
    APP.run(host= '0.0.0.0')


def main():
    (devices, id_to_vars) = parse_tb()

    build_trace_json(devices, id_to_vars)

    run_html_server(devices, id_to_vars)


if __name__ == "__main__":
    main()

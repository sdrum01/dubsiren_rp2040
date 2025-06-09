# Helper to load or save memory dumps from DUB-IY
#
# usage: 
# save single slots (firebutton)
# COMMAND: python3 dataTransfer.py -save xx
# xx = 0 - 16
# File will saves as JSON File with prefix receive_xx with autoenumerating prefix and date_time (e.g. receive_15_20250609_220727.json)
#
# save all 16 slots as dump
# COMMAND: python3 dataTransfer.py -save dump
# File will saved as JSON File with prefix receiveDump_xx with autoenumerating prefix and date_time (e.g. receiveDump_20241130.json)
# 
# load single file to one slot
# COMMAND: python3 dataTransfer.py -load xx receive_xx.json
# Content of receive_xx.json will load and store it to slot xx
# 
# load complete DUmpfile to all 16 slots
# COMMAND: python3 dataTransfer.py -load dump receiveDump_xxx.json
# Content of receiveDump_xxx.json will load and store it to slot 1 - 16


import serial
import time
import json
import sys
import os
import argparse
import serial.tools. list_ports
from datetime import datetime

def finde_arduino_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "Arduino" in port.description or "ttyACM" in port.device or "usbmodem" in port.device:
            return port.device
    return None


def read_json_from_serial(ser):
    buffer = ""
    start_time = time.time()

    while time.time() - start_time < 3:
        if ser.in_waiting:
            buffer += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            try:
                obj = json.loads(buffer.strip())
                return obj
            except json.JSONDecodeError:
                continue

    # Wenn kein gültiges JSON empfangen wurde, gib Rohinhalt aus
    print("❌ keine JSON-Antwort empfangen:")
    print("---- ROHDATEN BEGINN ----")
    print(buffer.strip())
    print("---- ROHDATEN ENDE ----")

    raise ValueError("Keine gültige JSON-Antwort erhalten")


def save_json_to_file(data, befehl):
    now = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{befehl}_{now}.json"
    script_dir = os.path.dirname(os.path.abspath(__file__))
    full_path = os.path.join(script_dir, filename)

    with open(full_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

    print(f"Dump gespeichert in: {full_path}")


def communicate(port, baudrate=115200, cmds=None):
    try:
        with serial.Serial(port, baudrate, timeout=2) as ser:
            time.sleep(2)

            for eintrag in (cmds or []):
                typ = eintrag[0]

                # ===============  RECEIVE / SAVE  ===============
                if typ == "receive":
                    befehl = eintrag[1]
                    print(f"Sende: {befehl}")
                    ser.reset_input_buffer()
                    ser.write((befehl + '\n').encode())
                    daten = read_json_from_serial(ser)
                    print("Geparst:", daten)
                    save_json_to_file(daten, befehl)

                # ===============  LOAD / SEND FILE  ===============
                elif typ == "load":
                    befehl, datei = eintrag[1], eintrag[2]
                    print(f"Lade '{datei}' nach {befehl}")
                    if not os.path.isfile(datei):
                        print("Datei nicht gefunden:", datei)
                        continue

                    ser.reset_input_buffer()
                    ser.write((befehl + '\n').encode())
                    time.sleep(0.1)                       # optional: kurze Pause
                    with open(datei, "r", encoding="utf-8") as f:
                        daten = json.load(f)  # JSON-Objekt laden

                    json_string = json.dumps(daten, separators=(',', ':'))  # kompaktes JSON
                    slot = befehl.split("_")[1] if "_" in befehl else "?"   # z.B. "load_1" → "1"
                    print(f"slot: {slot}, command: {befehl}")
                    if slot == "dump":
                        payload = f"load({json_string})\n" 
                    else: 
                        payload = f"load({slot}:{json_string})\n"
                    ser.write(payload.encode('utf-8'))
                    print(f"Gesendet: {payload.strip()}")

                    # Inhalt schicken
                    ser.write(b'\n')                      # Terminator
                    print("Datei übertragen.")

                    daten = read_json_from_serial(ser)
                    print("Response Geparst:", daten)

            if not cmds:
                print("Connection established but no Parameters.")
                print("")
                print("# usage: ")
                print("# save single slots (firebutton)")
                print("# COMMAND: python3 dataTransfer.py -save xx")
                print("# xx = 0 - 16")
                print("# File will saves as JSON File with prefix receive_xx with autoenumerating prefix and date_time (e.g. receive_15_20250609_220727.json)")
                print("# ")
                print("# save all 16 slots as dump")
                print("# COMMAND: python3 dataTransfer.py -save dump")
                print("# File will saved as JSON File with prefix receiveDump_xx with autoenumerating prefix and date_time (e.g. receiveDump_20241130.json)")
                print("# ")
                print("# load single file to one slot")
                print("# COMMAND: python3 dataTransfer.py -load xx receive_xx.json")
                print("# Content of receive_xx.json will load and store it to slot xx")
                print("# ")
                print("# load complete Dumpfile to all 16 slots")
                print("# COMMAND: python3 dataTransfer.py -load dump receiveDump_xxx.json")
                print("# Content of receiveDump_xxx.json will load and store it to slot 1 - 16")

    except Exception as e:
        print("Fehler:", e)



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="DUB-IY JSON-Dump-Receiver")
    parser.add_argument(
        "-save", type=str,
        help="Sendet 'receive_<x>' und speichert JSON (z. B. dump, 1, 2)"
    )
    parser.add_argument(
        "-load", nargs=2, metavar=("SLOT", "FILE"),
        help="Sendet 'load_<slot>' und überträgt <FILE> (z. B. 1 fire.json)"
    )
    args = parser.parse_args()

    befehle = []

    # SAVE (receive_x)
    if args.save:
        slot = args.save
        cmd = "receiveDump" if slot == "dump" else f"receive_{slot}"
        befehle.append(("receive", cmd))          # (Typ, Befehl)

    # LOAD (load_x + Datei)
    if args.load:
        slot, filename = args.load
        # cmd = "loadDump" if slot == "dump" else f"load_{slot}"
        cmd = f"load_{slot}"
        befehle.append(("load", cmd, filename))   # (Typ, Befehl, Datei)



    port = finde_arduino_port()
    if port:
        print("used port:", port)
        communicate(port, cmds=befehle)
    else:
        print("no valid serial port found.")
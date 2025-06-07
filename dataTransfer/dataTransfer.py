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

# def read_json_from_serial(ser):
#     buffer = ""
#     start_time = time.time()
#     while time.time() - start_time < 3:
#         if ser.in_waiting:
#             buffer += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
#             try:
#                 obj = json.loads(buffer.strip())
#                 return obj
#             except json.JSONDecodeError:
#                 continue
#     raise ValueError(f"Keine gültige JSON-Antwort erhalten")

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
    print("❌ Ungültige JSON-Antwort empfangen:")
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
                    payload = f"load({slot}:{json_string})\n"

                    ser.write(payload.encode('utf-8'))
                    print(f"Gesendet: {payload.strip()}")

                    # Inhalt schicken
                    ser.write(b'\n')                      # Terminator
                    print("Datei übertragen.")

                    daten = read_json_from_serial(ser)
                    print("Response Geparst:", daten)

            if not cmds:
                print("Verbindung steht – keine Aktion, da keine Parameter.")
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



    # parser = argparse.ArgumentParser(description="DUB-IY JSON-Dump-Receiver")
    # parser.add_argument('-savedump', action='store_true', help="Sendet 'receiveDump' und speichert JSON")
    # parser.add_argument('-save_1', action='store_true', help="Sendet 'receive_1' und speichert JSON")
    # args = parser.parse_args()

    befehle = []

    # SAVE (receive_x)
    if args.save:
        cmd = "receiveDump" if args.save == "dump" else f"receive_{args.save}"
        befehle.append(("receive", cmd))          # (Typ, Befehl)

    # LOAD (load_x + Datei)
    if args.load:
        slot, filename = args.load
        cmd = "loadDump" if slot == "dump" else f"load_{slot}"
        befehle.append(("load", cmd, filename))   # (Typ, Befehl, Datei)



    port = finde_arduino_port()
    if port:
        print("used port:", port)
        communicate(port, cmds=befehle)
    else:
        print("no valid serial port found.")
import serial
import time
import json
import sys
import os
import argparse
import serial.tools.list_ports
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
    raise ValueError("Keine gültige JSON-Antwort erhalten")

def save_json_to_file(data, befehl):
    now = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{befehl}_{now}.json"
    script_dir = os.path.dirname(os.path.abspath(__file__))
    full_path = os.path.join(script_dir, filename)

    with open(full_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

    print(f"Dump gespeichert in: {full_path}")


# def communicate(port, baudrate=115200, save=False):
#     try:
#         with serial.Serial(port, baudrate, timeout=2) as ser:
#             time.sleep(2)  # Warten auf Arduino-Reset

#             if save:
#                 ser.write(b'receiveDump\n')
#                 print("Befehl gesendet, warte auf Antwort...")

#                 daten = read_json_from_serial(ser)
#                 print("Geparst:", daten)
#                 save_json_to_file(daten)
#             else:
#                 print("Verbindung erfolgreich hergestellt (kein -savedump, daher keine Aktion).")

#     except Exception as e:
#         print("Fehler bei der seriellen Kommunikation:", e)


def communicate(port, baudrate=115200, cmds=None):
    try:
        with serial.Serial(port, baudrate, timeout=2) as ser:
            time.sleep(2)

            if cmds:
                for befehl in cmds:
                    print(f"Sende Befehl: {befehl}")
                    ser.reset_input_buffer()
                    ser.write((befehl + '\n').encode('utf-8'))

                    daten = read_json_from_serial(ser)
                    print("Geparst:", daten)
                    save_json_to_file(daten, befehl)
            else:
                print("Verbindung erfolgreich hergestellt (keine Aktion, da keine Argumente angegeben).")

    except Exception as e:
        print("Fehler bei der seriellen Kommunikation:", e)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="DUB-IY JSON-Dump-Receiver")
    # parser.add_argument('-savedump', action='store_true', help="Speichert den empfangenen Dump als JSON-Datei")
    parser.add_argument('-savedump', action='store_true', help="Sendet 'receiveDump' und speichert JSON")
    parser.add_argument('-save_1', action='store_true', help="Sendet 'receive_1' und speichert JSON")
    args = parser.parse_args()

    befehle = []
    if args.savedump:
        befehle.append("receiveDump")
    if args.save_1:
        befehle.append("receive_1")

    port = finde_arduino_port()
    if port:
        print("Verwende Port:", port)
        communicate(port, cmds=befehle)
    else:
        print("Kein Arduino-Port gefunden. Bitte manuell Port angeben.")
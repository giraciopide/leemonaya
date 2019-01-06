import sqlite3
import json
import time
import math
import hmac
import hashlib
import base64
from flask import Flask, request
from werkzeug.exceptions import BadRequest, Unauthorized

DB_PATH='leemonaya.sqlite3'
ENABLE_AUTHORIZATION = True
HMAC_KEY = bytearray('casdcasdcasdkjn12l3kjn412lkjdn1lkjnckajsd1234uh8ch9ch1wsjhv1co8', 'utf-8')

def initialize_schema_if_needed(db_path):
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS station_readings
            (station_id TEXT,
            tstamp INTEGER,
            payload TEXT)
    ''')
    conn.commit()
    conn.close()


def pprint_as_c_byte_array(key):
    ascii_codes = []
    for b in key:
        ascii_codes.append(str(b))

    return "#define HMAC_KEY_LENGTH %d\nstatic uint8_t hmac_key[HMAC_KEY_LENGTH] = {%s};" % (len(ascii_codes), ", ".join(ascii_codes));


def setup_app(db_path):
    print("""

/*** Cut and paste this HMAC KEY DEFINITION into your arduino sketch ***/
%s

    """ % (pprint_as_c_byte_array(HMAC_KEY)))
    initialize_schema_if_needed(db_path)
    return Flask(__name__)


app = setup_app(DB_PATH)


@app.route("/station-data", methods=['POST'])
def on_station_data_feed():
    (station_id, payload) = validate_station_reading(request)
    persist_station_data(station_id, payload)
    return ('', 204) # no content


def persist_station_data(station_id, payload, db_path=DB_PATH):
    json_payload = json.dumps(payload, sort_keys=True)
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    c.execute('''
       INSERT INTO station_readings VALUES (?, ?, ?)
    ''', [station_id, epoch_millis(), json_payload])
    c = conn.cursor()
    conn.commit()
    conn.close()


def validate_station_reading(req):

    raw_body = req.get_data()
    auth_header = req.headers.get('Authorization')

    if not auth_header:
        raise Unauthorized("Missing auth header")

    verify_signature(auth_header, raw_body)
    
    payload = req.get_json(force=True)
    
    station_id = payload['stationId']
    if not station_id:
        raise BadRequest('No stationId')

    return (station_id, payload)


def verify_signature(client_provided_base64_hash, raw_body, key=HMAC_KEY):
    hasher = hmac.new(key, raw_body, hashlib.sha256)
    server_computed_base64_hash = base64.b64encode(hasher.digest()).decode()
    print("Server computed hmac_sha256: [%s]" % (server_computed_base64_hash))
    print("Client provided hmac_sha256: [%s]" % (client_provided_base64_hash))
    if ENABLE_AUTHORIZATION and not (server_computed_base64_hash == client_provided_base64_hash):
        raise Unauthorized("Invalid signature")


def epoch_millis():
    return math.floor(time.time() * 1000.0)



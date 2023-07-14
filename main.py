from flask import Flask, jsonify, request, render_template
import sqlite3
from sqlite3 import Error
from datetime import datetime
from threading import Thread

app = Flask(__name__)

def connnectSQL():
    try:
        conn = sqlite3.connect('database/data.db')
        return conn
    except Error as e:
        print(e)

def generate_data(limit=30):
    while True:
        conn = connnectSQL()
        cur = conn.cursor()
        cur.execute("SELECT * FROM access_log ORDER BY id DESC LIMIT ?", (limit,))
        rows = cur.fetchall()
        conn.close()
        data = []
        for row in rows:
            data.append(row)
        return data

def is_card_id_exists(card_id):
    conn = connnectSQL()
    cur = conn.cursor()
    cur.execute("SELECT * FROM card_data WHERE card_id=?", (card_id,))
    rows = cur.fetchall()
    conn.close()
    if len(rows) == 0:
        return "false", "UNAUTHORIZED"
    else:
        card_holder = rows[0][2]
        return "true", card_holder

@app.route('/auth', methods=['GET'])
def auth():
    card_id = request.args.get('card_id')
    if card_id is None or card_id == "":
        return jsonify(granted="false", card_holder="UNAUTHORIZED")
    granted, card_holder = is_card_id_exists(request.args.get('card_id'))
    return jsonify(granted=granted, card_holder=card_holder)

@app.route('/log', methods=['GET'])
def log_access():
    time_now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    card_holder = request.args.get('card_holder')
    if card_holder is None or card_holder == "":
        return jsonify(status="failed")
    conn = connnectSQL()
    cur = conn.cursor()
    cur.execute("INSERT INTO access_log (card_holder, datetime) VALUES (?, ?)", (card_holder, time_now))
    conn.commit()
    conn.close()
    return jsonify(status="success")

@app.route('/get-data')
def get_data():
    token = request.args.get('token') or request.form.get('token')
    try:
        count = int(request.args.get('count')) or 30
    except Exception:
        count = 30
    if token != '1234':
        return jsonify({'error': 'Invalid password'}), 403
    # return jsonify(data[-count:])
    return jsonify(generate_data(count))

@app.route('/')
def index():
    return render_template('index.html', data=generate_data(30))

if __name__ == '__main__':
    data_thread = Thread(target=generate_data)
    data_thread.daemon = True
    data_thread.start()
    print("Dashboard Access Token: 1234")
    app.run(debug=True, host='0.0.0.0', port=3000)

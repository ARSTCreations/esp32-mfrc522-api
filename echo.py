from flask import Flask
from flask import jsonify
from flask import request
app = Flask(__name__)

@app.route('/echo', methods=['GET', 'POST'])
def echo():
    return jsonify(request.args)

@app.route('/true', methods=['GET', 'POST'])
def true():
    return jsonify(status="true")

app.run(debug=True)

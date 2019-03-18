#! /usr/bin/env python3
# coding: utf-8

"""
Ubuntu:
1. pip3 install pdfkit markdown
2. apt-get install wkhtmltopdf
   more platform: https://github.com/JazzCore/python-pdfkit/wiki/Installing-wkhtmltopdf

Windows：
1. scoop install python wkhtmltopdf
2. pip3 install pdfkit markdown
"""

import os
import re
import sys
import string

import pdfkit


def get_title(prefix, current):
    if prefix == "":
        title = "{}".format(current)
    else:
        title = "{}.{}".format(prefix, current)

    return title


def generate(prefix, line_list, current_sharp, start_index):
    size = len(line_list)
    if start_index >= size:
        return size + 1

    current = 1

    head_pattern = re.compile(r"^(#+)\s*(.*)$")
    title_pattern = re.compile(r"^[\s\.\d]+(.*)$")
    code_pattern = '```'
    code_cnt = 0

    i = start_index
    while True:
        if i >= size:
            break

        line = line_list[i]

        line_trip = line.strip()
        if line_trip.startswith(code_pattern):
            code_cnt += 1
        
        if code_cnt % 2 == 1:
            i += 1
            continue;

        match = head_pattern.match(line)
        if match:
            sharp = match.group(1)
            current_size = len(sharp)
            if current_size > current_sharp:
                i = generate(get_title(prefix, current - 1), line_list, current_size, i)

            elif current_size == current_sharp:
                title = get_title(prefix, current)
                right = match.group(2)
                r = title_pattern.match(right)
                if r:
                    right = r.group(1)

                if right != "":
                    line_list[i] = "{} {} {}".format(sharp, title, right)
                else:
                    line_list[i] = "{} {}".format(sharp, title)

                current += 1
                i += 1

            else:
                break

        else:
            i += 1

    return i


def reformat_md(filename):
    if not os.path.exists(filename):
        print ("%s file is not exist." % (filename))
        return

    with open(filename, mode="r", encoding="utf-8") as f:
        contents = f.read()

    line_list = contents.split("\n")
    generate("", line_list, 1, 0)

    with open(filename, "w", encoding="utf-8") as f:
        size = len(line_list)
        i = 0
        while i < size - 1:
            f.write(line_list[i])
            f.write("\n")
            i += 1

        f.write(line_list[size-1])


# 设置环境变量
def to_pdf(file_name):
    import markdown

    out_file = '.'.join([file_name.rsplit('.', 1)[0], 'pdf'])
    out_html = '.'.join([file_name.rsplit('.', 1)[0], 'html'])

    input_file = open(file_name, mode="r", encoding="utf-8")
    text = input_file.read()

    exts = ['markdown.extensions.extra', 'markdown.extensions.codehilite','markdown.extensions.tables','markdown.extensions.toc', 'markdown.extensions.nl2br']

    html = markdown.markdown(text, extensions=exts)

    output_file = open(out_html, "w",encoding="utf-8",errors="xmlcharrefreplace")

    # 可以根据选择修改css样式， 可以参考github.css和default.css
    css = '''
    <html">
    <head>
    <meta content="text/html; charset=utf-8" http-equiv="content-type" />
        <style>
            font-family: Helvetica, arial, sans-serif;
            font-size: 14px;
            line-height: 1.6;
            padding-top: 10px;
            padding-bottom: 10px;
            background-color: white;
            padding: 30px; }

            h1, h2, h3, h4, h5, h6 {
              margin: 20px 0 10px;
              padding: 0;
              font-weight: bold;
              -webkit-font-smoothing: antialiased;
              cursor: text;
              position: relative; }

            h1:hover a.anchor, h2:hover a.anchor, h3:hover a.anchor, h4:hover a.anchor, h5:hover a.anchor, h6:hover a.anchor {
              background: url("../../images/modules/styleguide/para.png") no-repeat 10px center;
              text-decoration: none; }

            h1 tt, h1 code {
              font-size: inherit; }

            h2 tt, h2 code {
              font-size: inherit; }

            h3 tt, h3 code {
              font-size: inherit; }

            h4 tt, h4 code {
              font-size: inherit; }

            h5 tt, h5 code {
              font-size: inherit; }

            h6 tt, h6 code {
              font-size: inherit; }

            h1 {
              font-size: 28px;
              color: black; }

            h2 {
              font-size: 24px;
              border-bottom: 1px solid #cccccc;
              color: black; }

            h3 {
              font-size: 18px; }

            h4 {
              font-size: 16px; }

            h5 {
              font-size: 14px; }

            h6 {
              color: #777777;
              font-size: 14px; }

            table {
              padding: 0; }
              table tr {
                border-top: 1px solid #cccccc;
                background-color: white;
                margin: 0;
                padding: 0; }
                table tr:nth-child(2n) {
                  background-color: #f8f8f8; }
                table tr th {
                  font-weight: bold;
                  border: 1px solid #cccccc;
                  text-align: left;
                  margin: 0;
                  padding: 6px 13px; }
                table tr td {
                  border: 1px solid #cccccc;
                  text-align: left;
                  margin: 0;
                  padding: 6px 13px; }
                table tr th :first-child, table tr td :first-child {
                  margin-top: 0; }
                table tr th :last-child, table tr td :last-child {
                  margin-bottom: 0;
        </style>
    </head>
    <body>
    %s
    </body>
    </html>
    '''

    output_file.write(css % html)
    output_file.close()


    pdfkit.from_file(out_html, out_file)

    os.remove(out_html)

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        print ("Help >\n" +
			"> generate sequece number: markdown-tools.py name.md>\n" +
			"> genereate pdf: markdown-tools.py pdf name.md>")
        os._exit(0)

    if sys.argv[1] == 'pdf':
        to_pdf(sys.argv[2])
        os._exit(0)

    reformat_md(sys.argv[1])

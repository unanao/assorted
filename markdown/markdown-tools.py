#! /usr/bin/env python3
# coding: utf-8

"""
INSTALL:
1. pdfkit
2. https://wkhtmltopdf.org/downloads.html
"""

import os
import re
import sys

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

    i = start_index
    while True:
        if i >= size:
            break

        line = line_list[i]
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

    with open(filename, "r") as f:
        contents = f.read()

    line_list = contents.split("\n")
    generate("", line_list, 1, 0)

    with open(filename, "w") as f:
        size = len(line_list)
        i = 0
        while i < size - 1:
            f.write(line_list[i])
            f.write("\n")
            i += 1

        f.write(line_list[size-1])


# 设置环境变量
def to_pdf(file_name):
    import codecs
    import markdown

    out_file = '.'.join([file_name.rsplit('.', 1)[0], 'pdf'])
    out_html = '.'.join([file_name.rsplit('.', 1)[0], 'html'])

    input_file = codecs.open(file_name, mode="r", encoding="utf-8")
    text = input_file.read()

    exts = ['markdown.extensions.extra', 'markdown.extensions.codehilite','markdown.extensions.tables','markdown.extensions.toc', 'markdown.extensions.nl2br']

    html = markdown.markdown(text, extensions=exts)


    output_file = codecs.open(out_html, "w",encoding="utf-8",errors="xmlcharrefreplace")

    github_css = os.path.dirname(__file__) + '/' + 'github.css'

    css = '''
    <html">
    <head>
    <meta content="text/html; charset=utf-8" http-equiv="content-type" />
    	<link href="
    ''' + github_css + '''" rel="stylesheet">
    </head>
    <body>
    %s
    </body>
    </html>
    '''

    output_file.write(css % html)
    output_file.close()

	
    pdfkit.from_file(out_html, out_file)


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

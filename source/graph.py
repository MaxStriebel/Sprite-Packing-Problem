from os import listdir
from os.path import join, isfile, splitext, basename
from matplotlib import pyplot as plt
from matplotlib.colors import ListedColormap
from matplotlib.lines import Line2D
from matplotlib.backends.backend_pdf import PdfPages
import pandas as pd
import numpy as np
from scipy.signal import argrelextrema
import argparse
from io import StringIO


def plotGraph(path, pdf, optimum):
    with open(path) as file:
        lines = file.readlines()
    header = pd.read_csv(StringIO('\n'.join(lines[:2])))
    data = pd.read_csv(StringIO('\n'.join(lines[2:])))
    iteration = data['iteration'].to_numpy()
    score = data['score'].to_numpy()
    overlap = data['overlap'].to_numpy()
    colorIndex = overlap
    colorIndex[colorIndex > 0] = 1

    settings = '\n'.join(['{}: {}'.format(name, values[0]) for name, values in header.items()])
    errorCount = sum(colorIndex)
    totalCount = len(colorIndex)
    best = min(score)
    stats = 'error: {}/{}\nmin: {}'.format(errorCount, totalCount, best)


    colormap = ListedColormap(["#071a56", "#e2060a"])
    fig, axis = plt.subplots(1, 1)
    axis.scatter(iteration, score, c = colorIndex, marker='x', cmap=colormap, linewidths=1)
    axis.axhline(optimum, color='black', dashes=[4, 2], linewidth=1, label='optimum')
    axis.set_ylim([0, 11.9 * optimum])
    fig.suptitle(basename(path))
    fig.text(0.68, 0.8, stats, verticalalignment='top')
    fig.text(0.4, 0.85, settings, verticalalignment='top')
    plt.legend()
    pdf.savefig()
    plt.close()
    
def plotImage(path, pdf):
    data = pd.read_csv(path).to_numpy()
    xMax = max(data[:, 0])
    xMin = min(data[:, 0])
    yMax = max(data[:, 1])
    yMin = min(data[:, 1])
    score = (xMax - xMin + 1) * (yMax - yMin + 1)
    image = np.zeros((yMax-yMin+1, xMax-xMin+1))
    alpha = np.zeros((yMax-yMin+1, xMax-xMin+1))
    for rowIndex in range(data.shape[0]):
        row = data[rowIndex, :]
        image[row[1]-yMin, row[0]-xMin] = row[2]
        alpha[row[1]-yMin, row[0]-xMin] = 1

    fig, axis = plt.subplots(1, 1)
    imshow = axis.imshow(image, cmap='plasma', alpha=alpha, extent=[xMin-.5, xMax+.5, yMax+.5, yMin-.5])
    localMinima0 = set(zip(* argrelextrema(image, np.less, axis=0, mode='wrap')))
    localMinima1 = set(zip(* argrelextrema(image, np.less, axis=1, mode='wrap')))
    localMinima = localMinima0.intersection(localMinima1)
    for (y, x) in localMinima:
        axis.text(x + xMin, y + yMin, "X", ha="center", va="center", color="w")
    fig.colorbar(imshow)
    fig.suptitle("{}\n{}".format(basename(path), score))
    pdf.savefig()
    plt.close()
    
def createPdf(directory, plot):
    files = [file for file in [join(directory, name) for name in listdir(directory)] if splitext(file)[1] == ".csv"]
    files.sort()
    with PdfPages(join(directory, 'plots.pdf')) as pdf:
        for file in files:
            plot(file, pdf)

# Main function. You don't have to change this
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('path')
    parser.add_argument('-t', '--type')
    parser.add_argument('-o', '--optimum', default=10000, type=int)
    args = parser.parse_args()
    if args.type == 'image':
        createPdf(args.path, plotImage)
    else:
        plot = (lambda a, b : plotGraph(a, b, args.optimum))
        createPdf(args.path, plot)

from os import listdir
from os.path import join, isfile, splitext, basename
from matplotlib import pyplot as plt
from matplotlib.colors import ListedColormap
from matplotlib.lines import Line2D
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.ticker as plticker
import pandas as pd
import numpy as np
from scipy.signal import argrelextrema
import argparse
from io import StringIO
from pathlib import Path


def plotGraph(path, showInfo):
    with open(path) as file:
        lines = file.readlines()
    header = pd.read_csv(StringIO('\n'.join(lines[:2])))
    data = pd.read_csv(StringIO('\n'.join(lines[2:])))
    iteration = data['iteration'].to_numpy()
    score = data['score'].to_numpy()
    overlap = data['overlap'].to_numpy()
    colorIndex = overlap
    colorIndex[colorIndex > 0] = 1

    colormap = ListedColormap(["#071a56", "#e2060a"])
    fig, axis = plt.subplots(1, 1)
    axis.scatter(iteration, score, c = colorIndex, marker='x', cmap=colormap, linewidths=1)
    axis.axhline(header['optimum'][0], color='black', dashes=[4, 2], linewidth=1, label='optimum')
    axis.set_xlabel('Sample index')
    axis.set_ylabel('Score')
    axis.set_ylim(0)

    valid = score[overlap == 0]
    validCount = len(valid)
    totalCount = len(colorIndex)
    best = min(valid)
    worst = max(valid)
    average = np.average(valid)
    median = np.median(valid)

    stats = 'valid: {}/{} min: {} max: {} avg: {} median: {}'.format(validCount, totalCount, best, worst, average, median)
    with open(path + '.txt', 'w') as f:
        f.write(stats)
    plt.legend()
    
def plotImage(path, showInfo):
    data = pd.read_csv(path).to_numpy()
    xMax = max(data[:, 0])
    xMin = min(data[:, 0])
    yMax = max(data[:, 1])
    yMin = min(data[:, 1])
    dimX = (xMax - xMin + 1)
    dimY = (yMax - yMin + 1)
    score =  dimX * dimY 
    image = np.zeros((dimY, dimX))
    alpha = np.zeros((dimY, dimX))
    for rowIndex in range(data.shape[0]):
        row = data[rowIndex, :]
        image[row[1]-yMin, row[0]-xMin] = row[2]
        alpha[row[1]-yMin, row[0]-xMin] = 1

    fig, axis = plt.subplots(1, 1)
    intervals = np.linspace(0.5, 8.5, 9)
    axis.set_xticks(np.linspace(0.5, dimX - 1.5, dimX - 1), minor=True)
    axis.set_yticks(np.linspace(0.5, dimY - 1.5, dimY - 1), minor=True)
    axis.tick_params(which='minor', bottom=showInfo, top=False, left=showInfo)
    axis.tick_params(which='major', bottom=False, top=False, left=False)
    axis.set_xlabel('Grid x')
    axis.set_ylabel('Grid y')
    imshow = axis.imshow(image, cmap='plasma', alpha=alpha, extent=[xMin-.5, xMax+.5, yMax+.5, yMin-.5])
    
def createPdf(directory, plot):
    files = [file for file in [join(directory, name) for name in listdir(directory)] if splitext(file)[1] == ".csv"]
    files.sort()
    with PdfPages(join(directory, 'plots.pdf')) as pdf:
        for file in files:
            plot(file)
            pdf.savefig()
            plt.close()

def createImages(directory, plot):
    files = [file for file in [join(directory, name) for name in listdir(directory)] if splitext(file)[1] == ".csv"]
    files.sort()
    for file in files:
        plot(file)
        savepath = "{}/{}.png".format(directory, Path(file).stem)
        plt.savefig(savepath, dpi=150, bbox_inches='tight', transparent=True)

# Main function. You don't have to change this
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('path')
    parser.add_argument('-t', '--type')
    parser.add_argument('-i', '--info', action='store_true')
    args = parser.parse_args()
    if args.type == 'image':
        plot = (lambda a : plotImage(a, args.info))
        createImages(args.path, plot)
    else:
        plot = (lambda a : plotGraph(a, args.info))
        createImages(args.path, plot)

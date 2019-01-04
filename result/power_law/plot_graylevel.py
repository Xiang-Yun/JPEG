import matplotlib.pyplot as plt


def plot_gray_level(txt="Y_graylevel.txt", title_name="lena_GrayLevel Original"):
    f = open(txt, 'r')
    lines = f.readlines()
    data = []

    for line in lines:
        data.append(int(line.split(',')[0]))
    
    plt.bar(range(len(data)), data)
    plt.title(title_name)
    plt.xlabel('Gray Level')
    plt.ylabel('Number of pixels')
    plt.show()



plot_gray_level()
plot_gray_level("Y_05_graylevel.txt", "lena_GrayLevel c=1.0,r=0.5")
plot_gray_level("Y_20_graylevel.txt", "lena_GrayLevel c=1.0,r=2.0")
plot_gray_level("IGS_Y_graylevel.txt", "lena_GrayLevel IGS")

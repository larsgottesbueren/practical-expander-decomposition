suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

plot <-
    ggplot(df, aes(x=edges, y=time, color=factor(phi), shape=factor(type))) +
    geom_point() +
    labs(y="Time (s)",
         x="Edge count",
         col=expression(phi),
         shape="Graph type")

ggsave(output_file, plot)

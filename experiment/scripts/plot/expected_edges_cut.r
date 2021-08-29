suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

plot <-
    ggplot(df, aes(x=edges_expected, y=edges_cut, shape=factor(phi))) +
    geom_point() +
    labs(y="Edges cut",
         x="Expected edges cut",
         col="Conductance")

ggsave(output_file, plot)

suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

partitionPlot <-
    ggplot(df, aes(x=phi, y=partitions, group=phi)) +
    geom_boxplot() +
    scale_x_log10(labels = scales::number) +
    geom_hline(aes(yintercept=expected_partitions, color="red"), show.legend = F) +
    facet_grid(graph ~ ., scales="free_y") +
    labs(x=expression(phi),
         y="Number of partitions")

edgesCutPlot <-
    ggplot(df, aes(x=phi, y=edges_cut, group=phi)) +
    geom_boxplot() +
    scale_x_log10(labels = scales::number) +
    geom_hline(aes(yintercept=expected_edges_cut, color="red"), show.legend = F) +
    facet_grid(graph ~ ., scales="free_y") +
    labs(x=expression(phi),
         y="Edges cut")

plot <-
    partitionPlot | edgesCutPlot

ggsave(output_file, plot)

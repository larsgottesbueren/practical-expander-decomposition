suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

plot <-
    ggplot(df, aes(x=edges, y=balance, color=factor(min_balance))) +
    geom_point() +
    labs(y="Cut balance",
         x="Edge count",
         col="Minimum balance") +
    facet_grid(default_strategy ~ .)

ggsave(output_file, plot)

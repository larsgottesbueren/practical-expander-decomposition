suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

plot <-
    ggplot(df, aes(x=iteration, y=potential, color=graph)) +
    scale_y_log10(labels = scales::number) +
    labs(y="Potential",
         x="Iteration",
         col="Graph type") +
    stat_summary(fun=median, geom="line")

ggsave(output_file, plot)

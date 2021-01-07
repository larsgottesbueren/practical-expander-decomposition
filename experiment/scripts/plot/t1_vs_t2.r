suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

plot <-
    ggplot(df, aes(x=t1, y=t2)) +
    geom_point(alpha=0.5) +
    labs(x=expression(t[1]),
         y=expression(t[2]),
         title=expression(paste("Parameters ", t[1], " and ", t[2], " which find correct cut")))

ggsave(output_file, plot)

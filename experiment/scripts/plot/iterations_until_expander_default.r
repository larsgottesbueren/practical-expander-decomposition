suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))
suppressMessages(library(ggpmisc))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file) %>% filter(strategy == "Default")

f <- y ~ x

plot <-
    ggplot(df, aes(x=log10_squared_edges,
                   y=iterations,
                   col=graph_type)) +
    geom_point() +
    geom_smooth(method="lm", formula=f, fullrange=TRUE) +
    stat_poly_eq(aes(label = ..eq.label..), formula=f, parse=TRUE) +
    labs(y="Iterations",
         x=expression("log"[10]^2 ~ "m"),
         col='Graph type')

ggsave(output_file, plot)

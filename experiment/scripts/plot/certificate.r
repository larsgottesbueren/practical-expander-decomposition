suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

plot <-
    ggplot(df, aes(x=phi, y=certificate)) +
    geom_point() +
    labs(x=expression(phi),
         y=expression("Certificate" ~ phi[c])) +
    facet_wrap(~ graph, label="label_parsed", scales="free_y") +
    scale_x_continuous(breaks=c(0.0001, 0.0025, 0.005, 0.0075, 0.01), limits = c(0.0001, NA)) +
    theme(panel.spacing = unit(2, "lines"))

ggsave(output_file, plot)
